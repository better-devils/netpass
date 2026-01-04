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
  .asciiz "https://nppl.api.netpass.cafe/boss"

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

.org 0x13ade0
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

.org 0x10d6c4
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

.org 0x12a948
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
; If it starts with the initialNPDLUrl (https://npdl.cdn.nintendowifi.net/), replace it with newNPDLUrl (https://npdl.api.netpass.cafe/boss).
; This way, we redirect SpotPass NPDL requests to our server, 

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
  b BranchToPatchNpdlUrl

ConvertAakamaitoNPDL_NewSecondEnding:
  push {r4, r5, r6, r7, lr}
  blx strncpy ; Call the original method

BranchToPatchNpdlUrl:
  push {r0, r1, r2, r3}
  bl PatchNpdlUrl
.endarea

; This function essentially:
; - Gets the URL buffer size
; - Checks if the URL is an NPDL link, which are used by games to get SpotPass data
;   - If not:
;     - Leave
;   - If yes:
;     - Copy the current URL on a temporary buffer
;     - Build the patched URL from our URL and the one in the temporary buffer into the original URL buffer

.org 0x10BBCC
.area 0x1C
.db 0, 0 ; zero-termination of "string"
PatchNpdlUrl:
  ldr r6, [URLBufferSizePtrPtr]
  ldr r6, [r6]

  bl CallPatchNpdlUrl_cont1

.align
URLBufferSizePtrPtr:
  .word URLBufferSizePtr ; The URL buffer's size is fixed and stored in memory. Let's get it from there instead of re-hardcoding it
.endarea

.org 0x111878
.area 0x2C
.db 0, 0 ; zero-termination of "string"
CallPatchNpdlUrl_cont1:
  ; We want 4 + (URL buffer size) bytes of temporary memory in the stack.
  ; Unfortunately, "sub sp, r6" is unsupported by the 3DS. As a solution, add the negative equivalent of r6
  neg r6, r6
  add sp, r6
  neg r6, r6

  sub sp, 4

  ; We want to keep the pointer to the URL buffer and to the temporary buffer somewhere persistent

  mov r4, r7 ; URL buffer (passed by the parent function)
  add r5, sp, 0x4 ; Temporary buffer 

  ; Get the length of the URL prefix we want to patch

  ldr r0, [initialNPDLUrlPtr] ; Original prefix pointer
  blx strlen

  mov r7, r0 ; Save its length in r7

  ; Compare the r7 bytes of the URL buffer with the URL prefix to patch, so we can see if the URL we want to use needs to be patched

  mov r0, r4
  ldr r1, [initialNPDLUrlPtr] ; Original prefix
  mov r2, r7 ; Length of the original prefix
  blx strncmp

  bl CallPatchNpdlUrl_cont2

.align
initialNPDLUrlPtr:
  .word initialNPDLUrl

.endarea

.org 0x113694
.area 0x24
.db 0, 0 ; zero-termination of "string"
CallPatchNpdlUrl_cont2:
  ; If the URL in the URL buffer does not need patching, then skip the rest of the function

  cmp r0, 0
  bne skipPatchNpdlUrl

  ; If the URL in the URL buffer needs to be patched, copy it in the temporary buffer

  mov r0, r5 ; Temporary buffer
  mov r1, r4 ; URL buffer
  mov r2, r6 ; URL/Temporary buffer size
  blx strncpy

  ; Concatenate the new URL prefix with the other part of the original URL into the URL buffer

  mov r0, r4 ; URL buffer
  mov r1, r6 ; URL/Temporary buffer size

  bl CallPatchNpdlUrl_cont3

skipPatchNpdlUrl:
  ; Move the stack pointer to its original position

  add sp, r6
  add sp, 4

  ; Restore the registers we've saved and return to the patched function

  pop {r0, r1, r2, r3}
  pop {r4, r5, r6, r7, pc}

.endarea

.org 0x113818
.area 0x2C
.db 0, 0 ; zero-termination of "string"
CallPatchNpdlUrl_cont3:
  ldr r2, [newNPDLUrlPatternPtr] ; Pattern

  ldr r3, [newNPDLUrlPtr] ; Custom BOSS URL
  add r4, r5, r7 ; Temporary buffer + length of the original prefix = rest of URL
  str r4, [sp, 0x0]

  bl snprintf

  ; We're done patching, move back to the end of the procedure
  
  bl skipPatchNpdlUrl

.align
newNPDLUrlPtr:
  .word newNPDLUrl
newNPDLUrlPatternPtr:
  .word newNPDLUrlPattern
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
;   - 0x13ade0
;   - 0x10d6c4
;   - 0x12a948
;  SpotPass Patches:
;   - 0x11E0E4
;   - 0x10BBCC
;   - 0x111878
;   - 0x113694
;   - 0x113818
; Free:
;  - 001138D8, 001154BC, 001225D0, 00122BEC, 00122F04, 00123158, 0012984C, 001299C0, 0012EC74, 0012F0A0, 0012F1A8, 0013B3FC


;;;
; Read-only data
;;;

.org 0x148a7c
.area 0x584
slotPathPattern:
  .asciiz "/config/netpass/log_spr/_%08lx_%08lx_%ld"
  .align 4
initialNPDLUrl:
  .asciiz "https://npdl.cdn.nintendowifi.net/"
  .align 4
newNPDLUrl:
  .asciiz "https://npdl.api.netpass.cafe/boss"
  .align 4
newNPDLUrlPattern:
  .asciiz "%s/%s"
  .align 4
.endarea

.close
