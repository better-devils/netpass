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
trampoline_entry equ 0x10e536

CecdsSprAddSlot equ 0x10f438
getFsUserHandle equ 0x126fa8
FsUserOpenArchive equ 0x126da0
FsUserOpenFile equ 0x118bf0
newFullFileFromHandle equ 0x126d6c
s_handle_fsuser_2 equ 0x14b1a8
FsUserCloseArchive equ 0x126d30
CreateFileBuffers equ 0x13d7d8


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

.org hpp_domain_addr
  .asciiz "api.netpass.cafe"

.org reports_url_addr
  .asciiz "https://api.netpass.cafe/npvk/reports"

.org trampoline_entry
  bl SaveSlotData

; executable data

CallArg1 equ 0
CallArg2 equ CallArg1 + 0x4
CallArg3 equ CallArg2 + 0x4
FullFilePtr equ CallArg3 + 0x4
FsFilePtr equ FullFilePtr + 0x4
ArchiveHandle equ FsFilePtr + 0x4
PathArgs equ ArchiveHandle + 0x8
SlotBuffer equ PathArgs + 0xC
SlotBufferSize equ SlotBuffer + 0x4
PathBuffer equ SlotBufferSize + 0x4
StackArgsSize equ PathBuffer + 0x34

.org 0x13ADE0
.area 0x98
.align 2
SaveSlotData:
  push {r4, r5, lr}
  mov r4, r1 ; buffer
  mov r5, r2 ; size
  ; call the original method
  bl CecdsSprAddSlot
  push {r0, r1, r2, r3}

  sub sp, StackArgsSize

  str r4, [sp, SlotBuffer]
  str r5, [sp, SlotBufferSize]
  ; now we can add our own method here
  ;bl CreateFileBuffers
  mov r0, 0
  str r0, [sp, FsFilePtr]
  str r0, [sp, FullFilePtr]

  ; first we open the sd mmc archive
  bl getFsUserHandle ; user handle is in r0 now
  str r0, [sp, PathArgs + 8] ; we need in r0 a pointer to the handle
  add r0, sp, PathArgs + 8
  add r1, sp, ArchiveHandle
  mov r2, 9 ; SDMC archive
  mov r3, 1 ; empty path type
  mov r4, 0
  str r4, [sp, CallArg3] ; this will be our empty string
  add r4, sp, CallArg3
  str r4, [sp, CallArg1] ; pointer to 0 for path
  mov r4, 1
  str r4, [sp, CallArg2] ; size=1 for path
  bl FsUserOpenArchive
  cmp r0, 0
  bcc fail1

  ; now we store it into the full handle
  ldr r3, [sp, ArchiveHandle + 4]
  ldr r2, [sp, ArchiveHandle]
  ldr r4, [FSUserHandlePtr]
  ldr r1, [r4]
  add r0, sp, FullFilePtr
  bl newFullFileFromHandle
  cmp r0, 0
  bcc fail1

  ; now we have the full file handle, and thus should be able to open the file we want

  ; build PathArgs
  ; first create the path str
  ldr r0, [sp, SlotBuffer]
  ldr r4, [r0, 0x04] ; size
  str r4, [sp, CallArg2]
  ldr r4, [r0, 0x30] ; transfer id
  str r4, [sp, CallArg1]
  ldr r3, [r0, 0x08] ; title id
  ldr r2, [slotPathPatternPtr] ; pattern string
  mov r1, 0x34 ; destination string length
  add r0, sp, PathBuffer ; destination string
  bl snprintf

  add r0, r0, 1 ; add the null character from snprintf
  str r0, [sp, PathArgs + 8] ; string length
  mov r4, 3 ; ascii path typeSlotPathStr
  str r4, [sp, PathArgs]
  add r0, sp, PathBuffer ; the newly created string
  str r0, [sp, PathArgs + 4]

  ; open the file
  mov r3, 0b111 ; open flags
  add r2, sp, PathArgs
  add r1, sp, FsFilePtr
  ldr r0, [sp, FullFilePtr]
  cmp r0, 0
  beq fail1
  ldr r4, [r0] ; the pointer to the open file method is in the
  ldr r4, [r4] ; first four bytes of FsFullFile
  blx r4
  cmp r0, 0
  bcc fail1

  ; write to the file
  mov r4, 1 ; update
  str r4, [sp, CallArg3]

  bl func_cont
fail1:
  bl fail
.align
FSUserHandlePtr:
  .word s_handle_fsuser_2
slotPathPatternPtr:
  .word slotPathPattern
.endarea

.org 0x10D6C4
.area 0x2C
.db 0, 0 ; zero-termination of "string"
.align 2
func_cont:
  ldr r4, [sp, SlotBufferSize]
  str r4, [sp, CallArg2]
  ldr r4, [sp, SlotBuffer]
  str r4, [sp, CallArg1]
  mov r3, 0 ; file offset
  mov r2, 0
  add r1, sp, PathArgs ; use this as temporary variable again
  ldr r0, [sp, FsFilePtr]
  cmp r0, 0 ; check for null pointer
  beq fail2

  ldr r4, [r0] ; the second entry in the LUT at the top is
  add r4, 1*4  ; the file write method
  ldr r4, [r4]
  blx r4
  cmp r0, 0
  bcc fail2

  bl func_cont2
fail2:
  bl fail
.endarea

.org 0x12A948
.area 0x32
.db 0, 0 ; zero-termination of "string"
func_cont2:

fail:
  ldr r0, [sp, FsFilePtr]
  cmp r0, 0 ; check for null pointer
  beq no_close_file
  ; close up the file
  ldr r4, [r0] ; the 12th entry in the LUT at the top is
  add r4, 12*4 ; the file close method
  ldr r4, [r4]
  blx r4
no_close_file:
  ldr r0, [sp, ArchiveHandle]
  ldr r1, [sp, ArchiveHandle+4]
  orr r0, r1
  cmp r0, 0
  beq no_close_archive
  
  ldr r2, [sp, ArchiveHandle]
  ldr r3, [sp, ArchiveHandle+4]
  mov r1, 0
  bl getFsUserHandle ; user handle is in r0 now
  str r0, [sp, PathArgs]
  add r0, sp, PathArgs
  bl FsUserCloseArchive
no_close_archive:
  add sp, StackArgsSize

  pop {r0, r1, r2, r3}
  pop {r4, r5, pc}


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
.area 0x6
  bl ConvertAakamaitoNPDL_NewFirstEnding
  pop {R3-R7, PC}
.endarea

.org BOSS_ConvertAakamaitoNPDL_SecondEnding
.area 0x6
  bl ConvertAakamaitoNPDL_NewSecondEnding
  pop {R3-R7, PC}
.endarea

; We need to push call-safe registers to maintain their call safety.
; Then, just call whichever function each ending was originally calling.
; After that, we can push non-call-safe registers so we can restore them later to make sure the function we patched returns the same values
; Finally, call our URL patching function!

.org 0x11E0E4
.area 0x18
.db 0, 0 ; zero-termination of "string"
ConvertAakamaitoNPDL_NewFirstEnding:
  push {r4, r5, r6, r7, lr}
  bl sub_10C104 ; Call the original method
  b BranchToPatchSpotpassUrl

ConvertAakamaitoNPDL_NewSecondEnding:
  push {r4, r5, r6, r7, lr}
  blx strncpy ; Call the original method

BranchToPatchSpotpassUrl:
  push {r0, r1, r2, r3}
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

  ; We want 0x14 + (URL buffer size) bytes of temporary memory in the stack.
  ; Unfortunately, "sub sp, r6" is unsupported by the 3DS. As a solution, add the negative equivalent of r6
  neg r6, r6
  add sp, r6
  neg r6, r6

  sub sp, 0x18 ; 8 bytes for the snprintf parameters, 0x10 bytes for the subdomain buffer

  ; We want to keep the pointer to the URL buffer and to the temporary buffer somewhere persistent

  mov r4, r7 ; URL buffer (passed by the parent function)
  add r5, sp, 0x18 ; Temporary buffer 

  mov r0, #0 ; r0 will be our reusable index

  bl PatchSpotpassUrl_cont1

.align
URLBufferSizePtrPtr:
  .word URLBufferSizePtr ; The URL buffer's size is fixed and stored in memory. Let's get it from there instead of re-hardcoding it
.endarea

.org 0x12981C
.area 0x30
.db 0, 0 ; zero-termination of "string"
PatchSpotpassUrl_cont1:
  ; Find the offset to the end of the protocol par of the URL

  ldrb r1, [r4, r0] ; Read the r0'th character of the URL buffer
  cmp r1, 0 ; Is it null?
  beq PatchSpotpassUrl_FindEndProtocolLoop_ExitNoProtocol

  b PatchSpotpassUrl_FindEndProtocolLoop_DoLoop ; Otherwise, start the loop

PatchSpotpassUrl_FindEndProtocolLoop_NextIteration:
  add r0, r0, 1 ; Increment the reusable index
  cmp r0, r6 ; Compare with the URL buffer size
  bge PatchSpotpassUrl_SkipPatch_Redirect1 ; If index >= URL buffer size, then we must break from the loop

PatchSpotpassUrl_FindEndProtocolLoop_DoLoop:
  add r1, r4, r0
  ldrb r1, [r1, 1] ; Read the (r0+1)'th character of the URL buffer

  cmp r1, '/' ; Is it a slash?
  beq PatchSpotpassUrl_FindEndProtocolLoop_NextIsSlash ; If so, go check for the rest

  cmp r1, 0 ; Is it null?
  beq PatchSpotpassUrl_FindEndProtocolLoop_ExitNoProtocol ; If so, there's no protocol in the URL, move on

  b PatchSpotpassUrl_FindEndProtocolLoop_NextIteration ; If neither or slash nor null, go to the next iteration

PatchSpotpassUrl_FindEndProtocolLoop_NextIsSlash:
  ldrb r1, [r4, r0] ; Read the r0'th character of the URL buffer
  cmp r1, '/' ; Is it a slash?
  bne PatchSpotpassUrl_FindEndProtocolLoop_NextIteration ; If not, move to the next iteration

  add r1, r0, 2 ; We just read two slashes in a row, so remember the index of the character right after it

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
  mov r0, r1 ; Here, either r1 is 0 from reading a null character, or it is the index to the subdomain

  ; Now, r0 contains the index to the subdomain part of the URL.
  ; Make sure it starts with "np"

  mov r2, r0 ; Back up the index of the subdomain

  ldrb r1, [r4, r0] ; Read the r0'th character of the URL buffer
  cmp r1, 'n' ; Is it an 'n'?
  bne PatchSpotpassUrl_SkipPatch_Redirect2 ; If not, we have nothing to patch, move on

  add r0, r0, 1 ; Increment the reusable index
  ldrb r1, [r4, r0] ; Read the r0'th character of the URL buffer
  cmp r1, 'p' ; Is it an 'p'?
  bne PatchSpotpassUrl_SkipPatch_Redirect2 ; If not, we have nothing to patch, move on

  bl PatchSpotpassUrl_cont3

PatchSpotpassUrl_SkipPatch_Redirect2:
  bl PatchSpotpassUrl_SkipPatch

.endarea

.org 0x113694
.area 0x24
.db 0, 0 ; zero-termination of "string"
PatchSpotpassUrl_cont3:
  ; Now that we know the subdomain starts with "np", loop until we find a dot so we can establish the index to the end of the subdomain

PatchSpotpassUrl_FindEndSubdomainLoop_NextIteration:
  add r0, r0, 1 ; Increment the reusable index
  cmp r0, r6
  bge PatchSpotpassUrl_SkipPatch_Redirect3

  ldrb r1, [r4, r0] ; Read the r0'th character of the URL buffer

  cmp r1, 0 ; Is it null?
  beq PatchSpotpassUrl_SkipPatch_Redirect3 ; If so, we have nothing to patch, move on
  
  cmp r1, '/' ; Is it a slash?
  beq PatchSpotpassUrl_SkipPatch_Redirect3 ; If so, we have nothing to patch, move on
  
  cmp r1, '.' ; Is it a dot?
  bne PatchSpotpassUrl_FindEndSubdomainLoop_NextIteration ; If not, move to the next iteration

PatchSpotpassUrl_FindEndSubdomainLoop_EndLoop:
  ; Now that we know where the subdomain starts and end within the URL, copy it to a buffer on the stack

  add r0, r0, 1 ; Skip the dot
  mov r7, r0 ; Back-up the index to the end of the subdomain

  bl PatchSpotpassUrl_cont4

PatchSpotpassUrl_SkipPatch_Redirect3:
  bl PatchSpotpassUrl_SkipPatch

.endarea

.org 0x113818
.area 0x2C
.db 0, 0 ; zero-termination of "string"
PatchSpotpassUrl_cont4:
  add r1, r4, r2 ; URL buffer + Subdomain index
  sub r2, r0, r2 ; End of subdomain index - Subdomain index
  add r0, sp, 8 ; Subdomain buffer
  blx strncpy

  ; Get the length of the domain we want to patch

  ldr r0, [initialSpotpassUrlPtr] ; Pointer to the original domain
  blx strlen
  mov r2, r0 ; Save its length into r2

  ; Check that the domain of the URL is the one we're targetting

  mov r0, r7 ; Get back the index to the end of the subdomain
  add r0, r4, r0 ; Get the pointer this corresponds to in the URL buffer
  ldr r1, [initialSpotpassUrlPtr] ; Index to compare to
  add r7, r7, r2 ; Make our index point to the path part of the URL
  blx strncmp

  bl PatchSpotpassUrl_cont5

.align
initialSpotpassUrlPtr:
  .word initialSpotpassUrl
  
.endarea

.org 0x1138D8
.area 0x34
.db 0, 0 ; zero-termination of "string"
PatchSpotpassUrl_cont5:
  cmp r0, 0 ; Is the result 0?
  bne PatchSpotpassUrl_SkipPatch ; If not, then it means we're not looking at the domain we want to patch, so move on
  
  ; Copy the URL to the temporary buffer

  mov r0, r5 ; Temporary buffer
  mov r1, r4 ; URL buffer
  mov r2, r6 ; URL/Temporary buffer size
  blx strncpy

  ; Merge the URL parts we want to rewrite it in the URL buffer

  mov r0, r4 ; URL Buffer
  mov r1, r6 ; URL/Temporary buffer size
  ldr r2, [newSpotpassUrlPatternPtr] ; Pattern
  ldr r3, [newSpotpassUrlPtr] ; Custom BOSS URL
  add r4, r5, r7 ; Temporary buffer + length of the original prefix = rest of URL
  str r4, [sp, 4]
  add r4, sp, 8 ; Subdomain buffer
  str r4, [sp, 0]
  bl snprintf
  
PatchSpotpassUrl_SkipPatch:
  ; Move the stack pointer to its original position

  add sp, r6
  add sp, 0x18

  ; Restore the registers we've saved and return to the patched function

  pop {r0, r1, r2, r3}
  pop {r4, r5, r6, r7, pc}

.align
newSpotpassUrlPtr:
  .word newSpotpassUrl
newSpotpassUrlPatternPtr:
  .word newSpotpassUrlPattern

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
; Free strings:
;  - 00111878, 001154BC, 001225D0, 00122BEC, 00122F04, 00123158, 0012984C, 001299C0, 0012EC74, 0012F0BC, 0012F1A8, 0012F1E0, 0013B3FC
;        0x2C,     0x24,     0x18,     0x18,     0x20,     0x20,     0x28,     0x2C,     0x18,     0x30,     0x38,     0x20,     0x18


;;;
; Read-only data
;;;

.org 0x148a7c
.area 0x584
slotPathPattern:
  .asciiz "/config/netpass/log_spr/_%08lx_%08lx_%ld"
  .align 4
initialSpotpassUrl:
  .asciiz "cdn.nintendowifi.net"
  .align 4
newSpotpassUrl:
  .asciiz "https://api.netpass.cafe/"
  .align 4
newSpotpassUrlPattern:
  .asciiz "%s%s%s"
  .align 4
.endarea

.close
