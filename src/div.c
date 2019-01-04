#include "internal.h"

// TODO: don't exit on zero
// TODO: strip trailing zeros from the denominator

/*
	This is an implementation of Donald Knuth's Algorithm D

	Algorithm D is based on long division however it is quite a bit
	different than the typical school-book algorithm method.

	One of the things that makes Algorithm D unique is that it makes
	use of arrays that start at the first element instead of the zeroth.
	In this way, carries are actually put into the zeroth element instead
	of any need for the arrays to be expanded -- leading to very concise 
	code.

	Algorithm D has a few other steps that separate it from typical 
	school-book long division. 
		* A normalization step is provided
			This step is of the form such that a new numerator
			and denominator are derived via:
				n = base / N[0] + 1
				N * n = (new_numerator)
				d = base / D[0] + 1
				D * d = (new_denominator)
				N/D = new_numerator/new_denominator
			The effect of dividing the new numerator and
			denominator is identical to that of the original

		* 2 guesses are made early in the guessing stategem
		* Finally, the answer is derived from a long multiplication
		  followed by a long subtraction and a long addition.
		  
	Care was taken to reproduce Knuth's original algorithm which was
	written in MIX theoretical assembly. Wherever possible, I tried to
	use Knuth's variable naming conventios and duplicate his usage of
	goto statements in order to produce an educationally oriented
	interpretation that emphasizes his methods.

	I have omitted the final step of calculating the modulus operation
	for the sake of efficiency, though it should probably be added in
	at some future point. This would likely produce a speedup over
	arbitraire's current method which implements modulus in terms of

		modulus(a) = a - (b * (a / b))
	
	see src/modulo.c for this operation.
*/

void shmul(UARBT *num, size_t size, UARBT digit, UARBT *result, int base)
{
	int carry, value;
	size_t i = 0;

	if (digit == 0) {
		_arb_memset(result, 0, size);
	} else if (digit == 1) {
		_arb_copy_core(result, num, size);
	} else {
		for (carry = 0, i = size ;i > 0; i--)
		{
			value = num[i-1] * digit + carry;
			result[i-1] = value % base;
			carry = value / base;
		}
		if (carry != 0)
			result[i-1] = carry;
	}
}

int _long_sub(UARBT *u, size_t i, UARBT *v, size_t k, int b)
{ 
	int borrow = 0;
	int val = 0;
	for (;k+1 > 0; i--, k--) {
		val = u[i] - v[k] - borrow; 
		borrow = 0;
		if (val < 0) {
			val += b;
			borrow = 1;
		}
		u[i] = val;
	} 
	return borrow;
}

int _long_add(UARBT *u, size_t i, UARBT *v, size_t k, int b)
{
	int carry = 0;
	int val = 0;
	for (;k+1 > 0; i--, k--) { 
		val = u[i] + v[k] + carry;
		carry = 0;
		if (val >= b) {
			val -= b;
			carry = 1;
		}
		u[i] = val;
	}
	return carry;
}

fxdpnt *arb_div_inter(fxdpnt *num, fxdpnt *den, fxdpnt *q, int b, size_t scale)
{
	UARBT *u = NULL;
	UARBT *v = NULL;
	UARBT *vf = NULL;
	UARBT qg = 0;
	UARBT *temp = NULL;
	UARBT norm = 0;
	ssize_t uscal = 0;
	int out_of_scale = 0;
	size_t quodig = scale + 1;
	size_t offset = 0;
	size_t lea = 0;
	size_t leb = 0;
	size_t i = 0;
	size_t j = 0;

	if (iszero(den) == 0)
		arb_error("divide by zero\n");
	
	lea = rl(num) + rr(den);

	uscal = rr(num) - rr(den);
	if (uscal < (ssize_t)scale)
		offset = scale - uscal;

	/* temporary variables for num and den -- because we must modify them */
	u = arb_calloc(1, (num->len + offset + 3) * sizeof(UARBT));
	_arb_copy_core(u + 1, num->number, (num->len));

	vf = v = arb_calloc(1, (den->len + offset + 3) * sizeof(UARBT));
	_arb_copy_core(v, den->number, (den->len));
	leb = den->len;

	temp = arb_malloc((den->len+1) * sizeof(UARBT));

	for (;*v == 0; v++, leb--);

	if (leb > lea+scale) 
		out_of_scale = 1; 
	else
		if (!(leb>lea))
			quodig = lea-leb+scale+1;

	q->lp = quodig-scale;
	q->len = q->lp + scale;

	if (out_of_scale)
		goto end;

	if ((norm = (b / (v[0] + 1))) != 1){
		shmul(u, lea+uscal+offset+1, norm, u, b);
		shmul(v, leb, norm, v, b);
	}

	if (leb > lea)
		j = (leb-lea);
	
	for (qg = b-1;i <= lea+scale-leb; ++i, ++j, qg = b-1) {
		if (v[0] != u[i])
			qg = (u[i]*b + u[i+1]) / v[0];
		if (v[1] * qg > (u[i] * b + u[i+1] - v[0] * qg) * b + u[i+2]) {
			qg = qg - 1;
			if (v[1] * qg > (u[i] * b + u[i+1] - v[0] * qg) * b + u[i+2])
				qg = qg - 1;
		} 
		/* D4. [Multiply and Subtract] */
		if (qg != 0) {
			arb_mul_core(v, leb, &qg, 1, temp, b);
			if (!(_long_sub(u+leb, i, temp, leb, b)))
				goto D7;
			qg = qg - 1;
			if (_long_add(u+leb, i, v, leb-1, b))
				u[0] = 0; 
		}
		D7: /* D7 */
		q->number[j] = qg;
	}
	end:
	q = remove_leading_zeros(q);
	free(temp);
	free(u);
	free(vf);
	return q;
}

fxdpnt *arb_div(fxdpnt *a, fxdpnt *b, fxdpnt *c, int base, size_t scale)
{
	fxdpnt *c2 = arb_expand(NULL, a->len + b->len + scale);
	arb_init(c2);
	arb_setsign(a, b, c2);
	c2 = arb_div_inter(a, b, c2, base, scale);
	arb_free(c);
	return c2;
}

void divv(fxdpnt *num, fxdpnt *den, fxdpnt **c, int b, size_t scale, char *m)
{
	_internal_debug;
	*c = arb_div(num, den, *c, b, scale);
	_internal_debug_end;
}

