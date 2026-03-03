.3ds
.arm
.open "code.bin", "build/patched_code.bin", 0x100000

; We will patch the image share url when opening the webbrowser to our own
; so that people can still use the image share buttons easily

is_home_menu_applet_id equ 0x10ee10


; first our trampoline
.org 0x107794
  bl trampoline 

memregion equ 0x101078
.org memregion
.area 0x114
  mov r0, 0
  bx lr

trampoline:
  push {r0, r1, r2, r3, r4}
  ; r0, r1 - applet id
  ; r2 - param
  ; r3 - param_len
  
  cmp r0, 0x114
  bne fail ; fail if it isn't the web browser
  
  cmp r3, 23
  blt fail ; fail if the parameter is not long enough
  
  mov r4, r2 ; save the url
  ldr r0, [test_url_ptr]
  mov r1, r2
memsearch:
  ldr r2, [r0]
  and r2, 0xFF
  cmp r2, 0
  beq memsearch_done
  ldr r3, [r1]
  and r3, 0xFF
  cmp r2, r3
  bne fail
  ; the url is not the thing
  add r0, 1
  add r1, 1
  b memsearch
memsearch_done:

  ldr r1, [replace_url_ptr]
strcpy:
  ldr r0, [r1]
  and r0, 0xFF
  str r0, [r4]
  add r1, 1
  add r4, 1
  cmp r0, 0
  bne strcpy

fail:
  pop {r0, r1, r2, r3, r4}
  b is_home_menu_applet_id
test_url_ptr:
  .word test_url
test_url:
  .asciiz "https://i.nintendo.net"
replace_url_ptr:
  .word replace_url
replace_url:
  .asciiz "http://i.netpass.cafe"
.endarea

.close
