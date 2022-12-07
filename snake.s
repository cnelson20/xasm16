; Variables ;
Default_irq_handler = $20

SNAKE_ARRAY_X = $7000
SNAKE_ARRAY_Y = $7100
SNAKE_ARRAY_BEGIN = $30 ; 1 byte 
	
SNAKE_HEAD_X = $31
SNAKE_HEAD_Y = $32
SNAKE_LENGTH = $33

SNAKE_X_DIRECTION = $34
SNAKE_Y_DIRECTION = $35

FOOD_X = $36
FOOD_Y = $37 

PLAYER_ALIVE = $38 

LASTJIFFY = $39

; Constants 
SNAKE_COLOR = $06
EMPTY_COLOR = $00
FOOD_COLOR = $05

FILLED_SPACE = $A0


;.SEGMENT "INIT"
;.SEGMENT "STARTUP"
;.SEGMENT "ONCE"
	lda #0
	sta $9F35
	
	lda #FILLED_SPACE
	ldx #EMPTY_COLOR
	ldy #$10
	sty $9F22 
	stx $9F21 
LoopFillY:
	ldy #$0
	sty $9F20 
LoopFillX:
	sta $9F23 
	stx $9F23 
	ldy $9F20 
	cpy #160
	bcc LoopFillX
	ldy $9F21 
	iny 
	sty $9F21 
	cpy #60
	bcc LoopFillY
	
	lda #39 
	sta SNAKE_HEAD_X
	lda #29
	sta SNAKE_HEAD_Y
	lda #0
	sta SNAKE_Y_DIRECTION 
	inc A
	sta SNAKE_X_DIRECTION
	inc A 
	sta SNAKE_LENGTH
	
	jsr gen_food_xy

	;jsr preserve_default_irq ; sets custom as well

	;lda #0
;Loop:
	;beq Loop
	
frame:
	; keyboard controls 
	jsr $FFE4
	beq keyboard_done ; if no key pressed just branch
	cmp #$41 ; A 
	bne notAPressed
	
	lda SNAKE_X_DIRECTION
	cmp #1
	beq notAPressed
	lda #$FF
	sta SNAKE_X_DIRECTION
	lda #0
	sta SNAKE_Y_DIRECTION
notAPressed:
	cmp #$44 ; D
	bne notDPressed
	
	lda SNAKE_X_DIRECTION
	cmp #$FF 
	beq notDPressed
	lda #1
	sta SNAKE_X_DIRECTION
	lda #0
	sta SNAKE_Y_DIRECTION
notDPressed:
	cmp #$53 ; S
	bne notSPressed
	
	lda SNAKE_Y_DIRECTION
	cmp #$FF 
	beq notSPressed
	lda #0
	sta SNAKE_X_DIRECTION
	lda #1
	sta SNAKE_Y_DIRECTION
notSPressed:
	cmp #$57 ; W
	bne notWPressed
	
	lda SNAKE_Y_DIRECTION
	cmp #1
	beq notWPressed
	lda #$FF
	sta SNAKE_Y_DIRECTION
	lda #0
	sta SNAKE_X_DIRECTION
notWPressed:

	; end keyboard 
keyboard_done: 

	lda SNAKE_HEAD_X
	clc 
	adc SNAKE_X_DIRECTION
	sta SNAKE_HEAD_X
	tax 
	lda SNAKE_HEAD_Y 
	clc 
	adc SNAKE_Y_DIRECTION 
	
	sta SNAKE_HEAD_Y
	
	cpx #80
	bcs dead
	cmp #60
	bcc notDead
	
dead:
	;lda #$80
	;sta $9F25
	lda #0
	sta $01
	jmp ($FFFC)

notDead:
	lda SNAKE_HEAD_X
	cmp FOOD_X
	bne check_collision
	lda SNAKE_HEAD_Y
	cmp FOOD_Y
	bne check_collision
	
	inc SNAKE_LENGTH
	jsr gen_food_xy

check_collision:
	dec SNAKE_ARRAY_BEGIN
	
	ldx SNAKE_ARRAY_BEGIN
	inx 
	ldy #1 
checkPlayerLoop:
	lda SNAKE_ARRAY_X , X
	cmp SNAKE_HEAD_X
	bne incLoop
	lda SNAKE_ARRAY_Y , X
	cmp SNAKE_HEAD_Y
	bne incLoop
	beq dead

incLoop:	
	inx 
	iny 
	cpy SNAKE_LENGTH
	bcc checkPlayerLoop

draw_head:
	lda SNAKE_HEAD_X
	ldx SNAKE_ARRAY_BEGIN
	sta SNAKE_ARRAY_X, X
	asl 
	inc A 
	sta $9F20 
	lda SNAKE_HEAD_Y
	sta SNAKE_ARRAY_Y, X
	sta $9F21 
	lda #SNAKE_COLOR
	sta $9F23 

clear_tail:		
	lda SNAKE_ARRAY_BEGIN
	clc 
	adc SNAKE_LENGTH
	tax 
	lda SNAKE_ARRAY_X, X 
	sta $02
	asl
	inc A 
	sta $9F20 
	lda SNAKE_ARRAY_Y, X
	sta $9F21 
	sta $04
	lda #EMPTY_COLOR
	sta $9F23 
	
RDTIM = $FFDE 

	lda #2
	pha 
vsyncloop:
	pla 
	beq endloop
	dec A 
	pha 
waitvsync:
	jsr RDTIM
	sta LASTJIFFY
keep_waiting:
	jsr RDTIM
	cmp LASTJIFFY
	beq keep_waiting	
	
	jmp vsyncloop
endloop:
	jmp frame

gen_food_xy:
@entropy_get = $FECF
@get_x:
	jsr @entropy_get
	and #%01111111
	cmp #80
	bcs @get_x
	
	sta FOOD_X
@get_y:
	jsr $FECF 
	and #%00111111
	cmp #60
	bcs @get_y
	
	sta FOOD_Y
	
	ldy #0
	ldx SNAKE_ARRAY_BEGIN
checkFoodLoop:
	lda SNAKE_ARRAY_X, X
	cmp FOOD_X
	beq gen_food_xy
	lda SNAKE_ARRAY_Y, X
	cmp FOOD_Y
	beq gen_food_xy
	
	inx 
	iny 
	cpy SNAKE_LENGTH
	bcc checkFoodLoop
	
	lda FOOD_X
	asl 
	inc A 
	sta $9F20 
	lda FOOD_Y 
	sta $9F21 
	lda #FOOD_COLOR
	sta $9F23 
	rts