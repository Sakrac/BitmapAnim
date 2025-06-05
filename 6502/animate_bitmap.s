pool zpAnim $40-$80
pool zpLocal $80-$a0
zpAnim zpCurrAnim.w
zpAnim zpAnimPtr.w
zpAnim zpFramesLeft
zpAnim zpFrameWait
zpAnim zpAnimSpeed

{
	zpLocal .zpPtr.w
	zpLocal .zpScrn.w
	zpLocal .zpStripesLeft
	zpLocal .zpStripeChars

EmptyBGFrame:
;	iny
;	clc
	sec
	tya
	adc.z .zpPtr
	sta.z zpAnimPtr
	lda.z .zpPtr+1
	sta.z zpAnimPtr+1
	rts

DrawNextBGFrame:
	sta.z .zpPtr
	stx.z .zpPtr+1
	ldy #0
	beq DrawBGAnimFrame

; ax = ptr to anim to start
StartBGAnim:
	sta.z .zpPtr
	stx.z .zpPtr+1
	ldy #0
	lda (.zpPtr),y
	sta.z zpFramesLeft ; number of frames in animation
	iny
; .zpPtr+y = ptr to stripe count
DrawBGAnimFrame:
	lda.z zpAnimSpeed
	sta zpFrameWait
	lda.z (.zpPtr),y ; number of stripes
	beq EmptyBGFrame
	sta.z .zpStripesLeft
	iny ; probably won't overflow y here
;	bne .okIndex
;	inc .zpPtr+1
.okIndex
	; .zpPtr+y = start of a stripe
.nextStripe
	lda (.zpPtr),y
	sta.z .zpStripeChars
	iny
	lda (.zpPtr),y
	sta.z .zpScrn
	iny
	lda (.zpPtr),y
	sta.z .zpScrn+1
	{
		clc ; y+1-1 for reading address+1
		tya
		adc.z .zpPtr
		sta.z .zpPtr
		bcc %
		inc.z .zpPtr+1
	}
	ldy.z .zpStripeChars
	{
		lda (.zpPtr),y
		sta (.zpScrn),y
		dey
		bne !
	}
	ldy .zpStripeChars
	{
		clc ; +y for reading color data
		tya
		adc.z .zpPtr
		sta.z .zpPtr
		bcc %
		inc.z .zpPtr+1
	}
	lda.z .zpScrn+1
	ora #$d8
	sta.z .zpScrn+1
	{
		lda (.zpPtr),y
		sta (.zpScrn),y
		dey
		bne !
	}
	{
		sec ; should be at the bitmap address
		lda.z .zpStripeChars
		adc.z .zpPtr
		sta.z .zpPtr
		bcc %
		inc.z .zpPtr+1
	}
	ldy #0
	lda (.zpPtr),y
	sta.z .zpScrn ; bitmap address
	iny
	lda (.zpPtr),y
	sta.z .zpScrn+1
	{
		clc
		lda #2
		adc.z .zpPtr
		sta.z .zpPtr
		bcc %
		inc.z .zpPtr+1
	}
	ldy #0
	ldx.z .zpStripeChars
	{
		rept 8 {
			lda (.zpPtr),y
			sta (.zpScrn),y
			iny
		}
		dex
		bne !
	}
	{
		clc
		tya
		adc.z .zpPtr
		sta.z .zpPtr
		bcc %
		inc.z .zpPtr+1
	}
	ldy #0
	dec.z .zpStripesLeft
	beq .doneStripes
	jmp .nextStripe
.doneStripes
	; a = .zpPtr
	sta.z zpAnimPtr
	ldx.z .zpPtr+1
	stx.z zpAnimPtr+1
	rts
}

; wait time
; anim ptr, loop count, frame timing
; quit

AnimFrameWait:
	dc.b 1,0

AnimIndex:
	dc.b 0

Initialized:
	dc.b 0

Finished:
	dc.b 0

HotDogActive:
	dc.b 0

HotDogWait:
	dc.b 10 ; first wait

HotDogIndex:
	dc.b 0
