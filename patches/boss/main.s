.3ds
.thumb

.open "code.bin", "build/patched_code.bin", 0x100000


;;;
; Globals
;;;

snprintf equ 0x124b24


;;;
; StreetPass Relay (SPR) Patches
;;;

spr_url_addr equ 0x144048
hpp_domain_addr equ 0x147e59
reports_url_addr equ 0x10a3cc
spr_startup_time equ 0x1027be
spr_ap_filter_time equ 0x122968
boss_policy_url equ 0x1074f0

CecdsSprAddSlot equ 0x10f438
getFsUserHandle equ 0x126fa8
FsUserOpenArchive equ 0x126da0
FsUserOpenFile equ 0x118bf0
newArchiveFromHandle equ 0x126d6c
s_handle_fsuser_2 equ 0x14b1a8
s_handle_frdu equ 0x14b2f4
FsUserCloseArchive equ 0x126d30
CreateFileBuffers equ 0x13d7d8
GetThreadLocalStorage equ 0x127bc0
BuildAndWriteIpcHeader equ 0x127a90
SvcSendSyncRequest equ 0x127a88
create_string16 equ 0x121e98

CfgsGetLocalFriendCodeSeed equ 0x10d260
CecdsGetBossUserId equ 0x10a8b0
memclr equ 0x126ef0

.org spr_url_addr
;  .asciiz "https://devapi.netpass.cafe/spr"
  .asciiz "https://api.netpass.cafe/spr"

; set spr loop to ~15min
.org spr_startup_time
  .db 0x38

; unset the bssid locking
.org spr_ap_filter_time
  .dw 0

; set the policy list to our own url
.org boss_policy_url
  .asciiz "https://api.netpass.cafe/nppl"

.org reports_url_addr
  .asciiz "https://api.netpass.cafe/npvk/reports"

CallArg1 equ 0
CallArg2 equ CallArg1 + 0x4
CallArg3 equ CallArg2 + 0x4
FullFilePtr equ CallArg3 + 0x4
FsFilePtr equ FullFilePtr + 0x4
ArchiveHandle equ FsFilePtr + 0x4
PathArgs equ ArchiveHandle + 0x8
TempVar equ PathArgs + 0xC
StackArgsSize equ TempVar + 0x4

; We overwrite FrduGetMyPassword to instead read from a file in nand
.org 0x13a8bc ; FrduGetMyPassword
.area 2
  b get_nex_pwd_entry
FrduGetMyPassword_new_entry:
.endarea

; we need to set up a trampoline to dynamically set the base domain of
; the hpp url
.org 0x10f072
.area 4
  bl hpp_url_trampoline
.endarea

.org 0x13ADE0
.area 0x98
; this area is actually *not* a string but an unused area
get_nex_pwd_entry:
  push {r0, lr}
  bl UseCustomNex
  cmp r0, 0
  pop {r0}
  bne get_nex_pwd

  push {r0, r1, r4, r5, r6, r7}
  b FrduGetMyPassword_new_entry
get_nex_pwd:
; buffer is in r0, size is in r1
  push {r4, r5, r6}
  ; r5 will hold our result buffer
  mov r5, r0
  ; r6 will hold our result size
  mov r6, r1
  
  ; clear the input buffer
  blx memclr

  sub sp, StackArgsSize
  
  ; clear the stack
  mov r1, StackArgsSize
  add r0, sp, 0
  blx memclr
  
  ; first open the nand ArchiveHandle
  bl getFsUserHandle ; user handle is in r0 now
  str r0, [sp, PathArgs + 8] ; we need in r0 a pointer to the handle
  add r0, sp, PathArgs + 8
  add r1, sp, ArchiveHandle
  ldr r2, [nandFileType] ; nand_rw archive
  mov r3, 1 ; empty path type
  mov r4, 0
  str r4, [sp, CallArg3] ; this will be our empty string
  add r4, sp, CallArg3
  str r4, [sp, CallArg1] ; pointer to 0 for path
  mov r4, 1
  str r4, [sp, CallArg2] ; size=1 for path
  bl FsUserOpenArchive
  cmp r0, 0
  blt fail_get_nex_pwd0

  ; now we store the archive into the full handle
  ldr r3, [sp, ArchiveHandle + 4]
  ldr r2, [sp, ArchiveHandle]
  ldr r4, [FSUserHandlePtr]
  ldr r1, [r4]
  add r0, sp, FullFilePtr
  bl newArchiveFromHandle
  cmp r0, 0
  blt fail_get_nex_pwd0
  
  ; build the path args for the file
  ldr r0, [nidPwdPathPtr] ; file path
  str r0, [sp, PathArgs + 4]
  mov r0, 3 ; path type ascii string
  str r0, [sp, PathArgs]
  mov r0, nidPwdPathEnd - nidPwdPath ; the file length
  str r0, [sp, PathArgs + 8]
; open the file for reading
  mov r3, 0b1 ; open flags
  add r2, sp, PathArgs
  add r1, sp, FsFilePtr

  ldr r0, [sp, FullFilePtr]
  ldr r4, [r0] ; the pointer to the open file method is in the
  ldr r4, [r4] ; first four bytes of FsFullFile
  blx r4
  cmp r0, 0
  blt fail_get_nex_pwd0
  
; read from the file
  mov r4, r6 ; read buffer length
  str r4, [sp, CallArg2]
  mov r4, r5 ; read buffer
  str r4, [sp, CallArg1]
  mov r3, 0 ; file offset

  
  mov r2, 0
  add r1, sp, TempVar ; we don't care about how much we actually read
  
  bl get_nex_cont
fail_get_nex_pwd0:
  bl fail_get_nex_pwd
.align 4
nidPwdPathPtr:
  .word nidPwdPath
FSUserHandlePtr:
  .word s_handle_fsuser_2
nandFileType:
  .word 0x1234567D
.endarea

.org 0x12A948
.area 0x32
.db 0, 0 ; zero-termination of "string"
get_nex_cont:
  
  ldr r0, [sp, FsFilePtr]
  ;cmp r0, 0
  ;beq fail_get_nex_pwd
  ldr r4, [r0] ; the first entry in the LUT at the top is
  ldr r4, [r4] ; the file read method
  blx r4
  
  ; we fall through to fail to close the resorces
fail_get_nex_pwd:
  str r0, [sp, TempVar] ; save the result for function return
  ; close file
  ldr r0, [sp, FsFilePtr]
  cmp r0, 0 ; check for null pointer

  beq fail_get_nex_pwd_no_close_fsfileptr
  ; close up the file
  ldr r4, [r0] ; the 12th entry in the LUT at the top is
  add r4, 12*4 ; the file close method
  ldr r4, [r4]
  blx r4
  
  ldr r0, [sp, TempVar]
fail_get_nex_pwd_no_close_fsfileptr:
  str r0, [sp, TempVar]
  
  ldr r0, [sp, ArchiveHandle]
  ldr r1, [sp, ArchiveHandle+4]
  orr r0, r1
  cmp r0, 0
  beq fail_get_nex_pwd_no_close_archivehandle0
  ldr r2, [sp, ArchiveHandle]
  bl get_nex_cont_2
fail_get_nex_pwd_no_close_archivehandle0:
  bl fail_get_nex_pwd_no_close_archivehandle
.endarea

.org 0x10D6C4
.area 0x2C
.db 0, 0 ; zero-termination of "string"
get_nex_cont_2:
  ldr r3, [sp, ArchiveHandle+4]
  mov r1, 0
  add r0, sp, PathArgs + 8 ; user handle is in r0 now
  bl FsUserCloseArchive

fail_get_nex_pwd_no_close_archivehandle:
  ldr r0, [sp, TempVar] ; restore our result
  
  add sp, StackArgsSize ; restore stack pointer
  pop {r4, r5, r6, pc}

; our little trampoline to determine which hpp url
; we should be using
hpp_url_trampoline:
  push {r0, lr}
  bl UseCustomNex
  cmp r0, 0
  beq hpp_url_trampoline_skip
  ldr r1, [customHppDomainPtr]
hpp_url_trampoline_skip:
  pop {r0}
  bl create_string16
  pop {pc}
.align
customHppDomainPtr:
  .word customHppDomain
.endarea

;;;
; SpotPass Patches
;
; @author RSM (https://github.com/giroletm / https://gitlab.com/giroletm)
; @research 3dbrew contributors (https://www.3dbrew.org) ; DaniElectra (https://github.com/DaniElectra) ; RSM (https://github.com/giroletm / https://gitlab.com/giroletm)
;;;

; The function at 0x0010B1BC ("BOSS_ConvertAakamaitoNPDL") patches the requested URL to fix legacy akamai links. Let's add our own URL patch to it!
; We save the first subdomain of the URL. Then, we check if it starts with "np".
; If so, then make sure the rest of the domain name matches with initialSpotpassUrl (cdn.nintendowifi.net).
; Finally, build a new URL with newSpotpassUrl (https://api.netpass.cafe/) followed by the saved subdomain and then the original path
; This way, we redirect SpotPass requests to our server.
; For example, "https://npdl.cdn.nintendowifi.net/some/path" should be turned into "https://api.netpass.cafe/npdl/some/path"

; For Nintendo Video support we need to turn "http://pubXX-p.est.c.app.nintendowifi.net/some/path" into "https://api.netpass.cafe/pubXX-p/some/path"

strncmp equ 0x125148
strncpy equ 0x126C08
strlen equ 0x12775c

URLBufferSizePtr equ 0x10B208
sub_10C104 equ 0x10C104
BOSS_MakeHTTPRequest equ 0x122A2C

; BOSS_ConvertAakamaitoNPDL has two endings, patch both to redirect to somewhere we have more space to write custom code

BOSS_ConvertAakamaitoNPDL_FirstEnding equ 0x10B1F2
BOSS_ConvertAakamaitoNPDL_SecondEnding equ 0x10B1FE

.org BOSS_ConvertAakamaitoNPDL_FirstEnding
.area 0x4
  bl ConvertAakamaitoNPDL_NewFirstEnding
.endarea

.org BOSS_ConvertAakamaitoNPDL_SecondEnding
.area 0x4
  bl ConvertAakamaitoNPDL_NewSecondEnding
.endarea

; We need to push call-safe registers to maintain their call safety.
; Then, just call whichever function each ending was originally calling.
; After that, we can push non-call-safe registers so we can restore them later to make sure the function we patched returns the same values
; Finally, call our URL patching function!

.org 0x11E0E4
.area 0x18
.db 0, 0 ; zero-termination of "string"
ConvertAakamaitoNPDL_NewFirstEnding:
  push {r0-r8, lr}
  bl sub_10C104 ; Call the original method
  bl UseCustomNex
  cmp r0, 0
  bne ConvertAakamaitoNPDL_NewFirstEnding_do
  pop {r0-r8, pc}
ConvertAakamaitoNPDL_NewFirstEnding_do:
  bl PatchSpotpassUrl
.endarea

.org 0x1225D0
.area 0x18
.db 0, 0 ; zero-termination of "string"
ConvertAakamaitoNPDL_NewSecondEnding:
  push {r0-r8, lr}
  blx strncpy
  bl UseCustomNex
  cmp r0, 0
  bne ConvertAakamaitoNPDL_NewSecondEnding_do
  pop {r0-r8, pc}
ConvertAakamaitoNPDL_NewSecondEnding_do:
  bl PatchSpotpassUrl
.endarea

; This function essentially:
; - Finds the index to the end of the protocol part of the URL (after "https://" in the case of an HTTPS request, for example)
;   - If there isn't any protocol, then consider it's automatic and just use index 0
; - From this, checks if the subdomain starts with "np"
;   - If not:
;     - Leave
;   - If yes:
;     - Find the index of the end of the first subdomain part of the URL.
;     - From this, save the subdomain into a buffer, and check if the rest of the domain is Nintendo's CDN
;       - If not:
;         - Leave
;       - If yes:
;         - Buffer the URL into a temporary buffer
;         - Build a new URL from our URL, the buffered subdomain, and the aforementionned temporary buffer

.org 0x10BBCC
.area 0x1C
.db 0, 0 ; zero-termination of "string"
PatchSpotpassUrl:
  ldr r6, [URLBufferSizePtrPtr]
  ldr r6, [r6]

  mov r1, r6 ; total stack size in r1
  add r1, 0x18 ; two snprintf parameters and subdomain buffer is 0x10 large

  ; We want 0x14 + (URL buffer size) bytes of temporary memory in the stack.
  ; Unfortunately, "sub sp, r6" is unsupported by the 3DS. As a solution, add the negative equivalent of r6
  neg r0, r1
  add sp, r0

  ; clear the stack
  ; r1 already holds the stack size
  add r0, sp, 0
  blx memclr
  ; We want to keep the pointer to the URL buffer and to the temporary buffer somewhere persistent

  ; r4 - protocol offset backup
  ; r5 - current replace object pointer
  ; r6 - input buffer size
  ; r7 - url buffer

  bl PatchSpotpassUrl_cont1

.align
URLBufferSizePtrPtr:
  .word URLBufferSizePtr ; The URL buffer's size is fixed and stored in memory. Let's get it from there instead of re-hardcoding it
.endarea

.org 0x12981C
.area 0x30
.db 0, 0 ; zero-termination of "string"
PatchSpotpassUrl_cont1:
  mov r4, #0 ; we put the protocol offset into r4
  ; Find the offset to the end of the protocol par of the URL

  ldrb r1, [r7, r4] ; Read the r4'th character of the URL buffer
  cmp r1, 0 ; Is it null?
  beq PatchSpotpassUrl_FindEndProtocolLoop_ExitNoProtocol

  b PatchSpotpassUrl_FindEndProtocolLoop_DoLoop ; Otherwise, start the loop

PatchSpotpassUrl_FindEndProtocolLoop_NextIteration:
  add r4, r4, 1 ; Increment the reusable index
  cmp r4, r6 ; Compare with the URL buffer size
  bge PatchSpotpassUrl_SkipPatch_Redirect1 ; If index >= URL buffer size, then we must break from the loop

PatchSpotpassUrl_FindEndProtocolLoop_DoLoop:
  add r1, r7, r4
  ldrb r1, [r1, 1] ; Read the (r4+1)'th character of the URL buffer

  cmp r1, '/' ; Is it a slash?
  beq PatchSpotpassUrl_FindEndProtocolLoop_NextIsSlash ; If so, go check for the rest

  cmp r1, 0 ; Is it null?
  beq PatchSpotpassUrl_FindEndProtocolLoop_ExitNoProtocol ; If so, there's no protocol in the URL, move on

  b PatchSpotpassUrl_FindEndProtocolLoop_NextIteration ; If neither or slash nor null, go to the next iteration

PatchSpotpassUrl_FindEndProtocolLoop_NextIsSlash:
  ldrb r1, [r7, r4] ; Read the r4'th character of the URL buffer
  cmp r1, '/' ; Is it a slash?
  bne PatchSpotpassUrl_FindEndProtocolLoop_NextIteration ; If not, move to the next iteration

  add r1, r4, 2 ; We just read two slashes in a row, so remember the index of the character right after it

PatchSpotpassUrl_FindEndProtocolLoop_ExitNoProtocol:
PatchSpotpassUrl_FindEndProtocolLoop_ExitEndLoop:
  bl PatchSpotpassUrl_cont2

PatchSpotpassUrl_SkipPatch_Redirect1:
  bl PatchSpotpassUrl_SkipPatch

.endarea

.org 0x12F0A0
.area 0x1C
.db 0, 0 ; zero-termination of "string"
PatchSpotpassUrl_cont2:
  mov r4, r1 ; Here, either r1 is 0 from reading a null character, or it is the index to the subdomain

  ; Now, r4 contains the offset to the subdomain part

  ldr r5, [spotpassUrlRewritePtr]
  
  b PatchSpotpassUrl_LoopEntry
PatchSpotpassUrl_LoopContinue:
  add r5, r5, 7 ; iterate to the next object
  add r5, r5, 5

PatchSpotpassUrl_LoopEntry:
  ; first, we check if the subdomain part matches
  ldr r0, [r5, 0x0] ; the subdomain start match
  cmp r0, 0

  bl PatchSpotpassUrl_cont3
.align
spotpassUrlRewritePtr:
  .word spotpassUrlRewrite
.endarea

.org 0x113694
.area 0x24
.db 0, 0 ; zero-termination of "string"
PatchSpotpassUrl_cont3:
  beq PatchSpotpassUrl_SkipPatch_Redirect2 ; if we already checked all then do nothing further
  blx strlen
  
  mov r8, r0 ; back up the length of the subdomain match
  mov r2, r0
  add r0, r7, r4 ; get the start of the url
  ldr r1, [r5, 0x0] ; subdomain prefix to match against
  blx strncmp
  cmp r0, 0
  bne PatchSpotpassUrl_LoopContinue_Redirect1 ; no match

  bl PatchSpotpassUrl_cont4
PatchSpotpassUrl_SkipPatch_Redirect2:
  bl PatchSpotpassUrl_SkipPatch
PatchSpotpassUrl_LoopContinue_Redirect1:
  bl PatchSpotpassUrl_LoopContinue
.endarea

.org 0x113818
.area 0x2C
.db 0, 0 ; zero-termination of "string"
PatchSpotpassUrl_cont4:
  ; ok, we now know that the subdomain part matches. so, let's loop
  ; to the end of the subdomain
  mov r0, r8 ; length of subdomain match
  add r0, r0, r4 ; r0 now has our current counter / offset
PatchSpotpassUrl_FindEndSubdomainLoop_Continue:
  add r0, r0, 1
  ; if the next char is 0 or / we have nothing to patch
  ldrb r1, [r7, r0]
  cmp r1, 0
  beq PatchSpotpassUrl_LoopContinue_Redirect2
  cmp r1, '/'
  beq PatchSpotpassUrl_LoopContinue_Redirect2
  cmp r1, '.'
  bne PatchSpotpassUrl_FindEndSubdomainLoop_Continue
  add r0, r0, 1 ; skip the dot

  ; now we copy the subdomain buffer onto the backup
  mov r8, r0 ; back up the index to the end of the sub domain
  add r1, r7, r4 ; url buffer + subdomain index
  sub r2, r0, r4 ; end of subdomain index - subdomain index
  add r0, sp, 8 ; subdomain buffer
  bl PatchSpotpassUrl_cont5
PatchSpotpassUrl_LoopContinue_Redirect2:
  bl PatchSpotpassUrl_LoopContinue
.endarea

.org 0x1138D8
.area 0x34
.db 0, 0 ; zero-termination of "string"
PatchSpotpassUrl_cont5:
  blx strncpy
  ; now it is time to check if the domain part matches up
  ldr r0, [r5, 0x4]
  blx strlen


  mov r2, r0
  ldr r1, [r5, 0x4]
  mov r3, r8 ; offset to start of subdomain
  add r0, r3, r7
  
  add r3, r3, r2
  mov r8, r3 ; save the full offset into r8
  
  blx strncmp
  cmp r0, 0
  bne PatchSpotpassUrl_LoopContinue_Redirect2
  ; ok, it does start with the correct domain as well, so we have a url to patch!

  ; Copy the path to the temporary buffer
  mov r3, r8
  add r0, sp, 0x18 ; temporary buffer
  add r1, r7, r3 ; path of the url
  mov r2, r6 ; size of the url / temporary buffer
  blx strncpy

  bl PatchSpotpassUrl_cont6
.endarea

.org 0x111878
.area 0x2C
.db 0, 0 ; zero-termination of "string"
PatchSpotpassUrl_cont6:
  ; now time to merge all the URL parts together
  mov r0, r7 ; the url buffer
  mov r1, r6 ; buffer size
  ldr r2, [newSpotpassUrlPatternPtr] ; pattern
  ldr r3, [r5, 0x8] ; path prefix
  add r4, sp, 8 ; subdomain buffer
  str r4, [sp, 0]
  add r4, sp, 0x18 ; path buffer
  str r4, [sp, 4]
  bl snprintf
PatchSpotpassUrl_SkipPatch:
  ; Move the stack pointer to its original position
  
  add sp, r6
  add sp, 0x18
  
  ; Restore the registers we've saved and return to the patched function
  
  pop {r0-r8, pc}
.align
newSpotpassUrlPatternPtr:
  .word newSpotpassUrlPattern
.endarea

.org 0x1154BC
.area 0x24
.db 0, 0 ; zero-termination of "string"
FrduGetMyPlayingGame:
  push {r3, r4, lr}
  sub sp, 0x8
  blx GetThreadLocalStorage
  add r0, 0x80
  str r0, [sp, 0x4]
  mov r0, 0
  mov r3, r0
  mov r2, r0
  str r0, [sp]
  mov r1, 0xC
  add r0, sp, 4
  bl BuildAndWriteIpcHeader
  bl FrduGetMyPlayingGame_cont
.endarea

.org 0x122F04
.area 0x20
.db 0, 0 ; zero-termination of "string"
FrduGetMyPlayingGame_cont:
  ldr r0, [FrduHandlePtr]
  ldr r0, [r0]
  blx SvcSendSyncRequest
  cmp r0, 0
  blt _FrduGetMyPlayingGame_Exit
  ldr r0, [sp, 0x4]
  
  ldr r1, [r0, 8]
  ldr r2, [r0, 0xC]
  ldr r0, [r0, 4]
  
_FrduGetMyPlayingGame_Exit:
  add sp, 0x8
  pop {r3, r4, pc}
.align
FrduHandlePtr:
  .word s_handle_frdu
.endarea

.org 0x122BEC
.area 0x18
.db 0, 0 ; zero-termination of "string"

.endarea

.org 0x123158
.area 0x20
.db 0, 0 ; zero-termination of "string"

.endarea

.org 0x12F1A8
.area 0x38
.db 0, 0 ; zero-termination of "string"
UseCustomNex:
  push {r1, r2, lr}
  bl FrduGetMyPlayingGame
  cmp r0, 0
  blt UseCustomNex_Yes
  ldr r0, [common_title_upper]
  cmp r2, r0
  bne UseCustomNex_Yes
  ldr r2, [nexTitleExcludeListPtr]
UseCustomNex_loop:
  ldr r0, [r2]
  cmp r0, 0
  beq UseCustomNex_Yes
  cmp r1, r0
  beq UseCustomNex_No
  add r2, 4
  b UseCustomNex_loop
UseCustomNex_No:
  mov r0, 0
  pop {r1, r2, pc}
UseCustomNex_Yes:
  mov r0, 1
  pop {r1, r2, pc}
.align
common_title_upper:
  .word 0x00040000
nexTitleExcludeListPtr:
  .word nexTitleExcludeList
.endarea


; When the server returns a 30X (redirect), the path part is copied into the URL buffer, but with an artificial limit of 0x40 bytes, which breaks URLs considering it's super short.
; The buffer it's stored into already is larger, so just increase it that limit to 0xFC, it should have us covered.

.org 0x1218DE
.area 1
.db 0xFC
.endarea

; We could have also patched the RSA checks at the following pointers:
; - at 0x0010EA6C, the sig check happens for the BOSS content header, patch it away
; - at 0x0010ED58, the sig check happens for whetever else, patch it away
; Or better yet, make 00110A8C always return 1, but would it cause security issues?
; In the end, it doesn't matter! Luma3DS already patches the RSA verification function for other purposes, so no work to do there!


;;;
; Spots where custom code can be written in .text
;;;

; Already used:
;  StreetPass Relay (SPR) Patches:
;   - 0x13ADE0
;   - 0x10D6C4
;   - 0x12A948
;  SpotPass Patches:
;   - 0x11E0E4
;   - 0x10BBCC
;   - 0x12981C
;   - 0x12F0A0
;   - 0x113694
;   - 0x113818
;   - 0x1138D8
;   - 0x111878
;   - 0x1154BC
;   - 0x1225D0
;   - 0x122BEC
;   - 0x122F04
;   - 0x123158
;   - 0x12F1A8
; Free strings:
;  - 0012984C, 001299C0, 0012EC74, 0012F0BC, 0012F1E0, 0013B3FC
;        0x28,     0x2C,     0x18,     0x30,     0x20,     0x18


;;;
; Read-only data
;;;

.org 0x148a7c
.area 0x584
.align 4
spotpassUrlRewrite:
  .word spotpassGeneralPrefix
  .word spotpassGeneralUrl
  .word spotpassGeneralPath
  
  .word 0
  
  .word spotpassVideoPrefix
  .word spotpassVideoUrl
  .word sptopassVideoPath

  .word 0

spotpassGeneralPrefix:
  .asciiz "np"
spotpassGeneralUrl:
  .asciiz "cdn.nintendowifi.net"
spotpassGeneralPath:
  .asciiz "/"
spotpassVideoPrefix:
  .asciiz "pub"
spotpassVideoUrl:
  .asciiz "est.c.app.nintendowifi.net"
sptopassVideoPath:
  .asciiz "/v/"

  .align 4
nexTitleExcludeList:
  .word 0xC9B00 ; pokemon bank
;  .word 0x51800 ; letterbox (for testing only)
  .word 0
customHppDomain:
  .asciiz "api.netpass.cafe"
newSpotpassUrlPattern:
  .asciiz "https://api.netpass.cafe%s%s%s"
  .align 4
nidPwdPath:
  .asciiz "/netpass/nid_pwd.bin"
nidPwdPathEnd:
  .align 4
.endarea

.close
