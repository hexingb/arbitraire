#include "internal.h"
/*
	This function is under construction.


               75.29
            ________
          \/ 5669.00
             49
             _______
7*20 = 145  | 769
         -    725
             _______
75*20= 1502 |  44 00
          -    30 04
              _______
752*20=15049 | 13 9600
           -   13 5441
                _________
7529*20=150582 |  315900
             -

            23.8
          _______
        \/5 69.00
          4
         _______       44 * 4 = 176
     43 | 1 69         43 * 3 = 129
          1 29
          ______       469 * 9 = 4221
     468 |  40.00      468 * 8 = 3744
            37.44

        * Get the first number set
        * attempt to find the square of the number set using multiplication
        * push the discovered number onto the answer
        * perform a subtraction and carry down 2 digits from the number that is being squared
        * multiply the answer by 2 and put into temp
        * find how many times temp can go into the carried down sub and its two digits


	* if the number has an even length you can get two digits but if the length
	 *  is odd then you'll need to start with one (though 3 may work??)

        while (1)
        {
                if ( num * num == num1 )
                {
                        break;
                } else if (num * num > num1 ) {
                        --num;
                        break;
                }
                ++num;


        }

	this style of loop should work for figuring out the squares

	
	There is no position agnostic way to generalize long hand
	square root

	for instance if you started with sqrt(5555)
	and tried to factor the first 5, you would get a "2"
	and this is incorrect because the sqrt of 5555 is
	74.531..., so you need to first start with the "55" in 
	order to derive a "7" which is the first correct digit

	1> factor the leading set
	2> square the initial factoziation and carry it down
	3> subtract the new guess from the primary
	4> multiply the entire estimated answer by 2 and put it to the side
	5> factor the side new guess up the last guess by manipulating
	   its final digit (this should be seamless with factor())
	6> take the last digit of the side new guess and push it
		onto the answer, then use that guessing factor and replace
		the primary guess
	7> carry down two digits
	8> subtract the new guess from the old guess
	9> repeat

	notes:
		instead of pushing the answer digits onto a stack we
	can use a pure maths method of multiplying by 10 (base) and
	then adding the number. this will be an expensive procedure
	but it can be optimized away at a later time

*/

fxdpnt *factor(fxdpnt *a, fxdpnt *b, int base, size_t scale)
{
	/* Handle sqrt factorization guesses of the form
		465n * n < guess. so the naive algorithm works fine
	*/

	fxdpnt *temp = arb_str2fxdpnt("+0.00");
	arb_copy(temp, a);
	int comp = -100;
	while (1)
	{
		arb_mul2(a, a, &temp, base, scale);
		comp = arb_compare(temp, b,  10);
		if (comp == 0) {
			break;
		} else if (comp > 0)
		{
			arb_sub2(a, one, &a, base);
			break;
		}
		arb_incr(&a, base);
	}
	return a;
}

void factor2(fxdpnt **a, fxdpnt *b, int base, size_t scale)
{
	*a = factor(*a, b, base, scale);
}

fxdpnt *factor_one(fxdpnt *a, fxdpnt *b, int base, size_t scale)
{ 
	fxdpnt *temp = arb_str2fxdpnt("+1");
	arb_copy(temp, a);
	int comp = -100;
	while (1)
	{ 
		a = arb_mul(a, temp, a, base, scale);
		comp = arb_compare(a, b,  10);
		if (comp == 0) {
			break;
		} else if (comp > 0)
		{
			arb_sub2(a, one, &a, base);
			break;
		}
		arb_incr(&temp, base);
		arb_incr(&a, base);
	}
	return a;
}

void pushon(fxdpnt *a, fxdpnt *b)
{
	arb_expand(a, a->len + b->len);
	memcpy(a->number + a->len, b->number, b->len * sizeof(ARBT));
	a->len += b->len;
	a->lp = a->len;
}
void addfront(fxdpnt *a, fxdpnt *b)
{
	arb_expand(a, b->len);
	memcpy(a->number, b->number, b->len * sizeof(ARBT));
}

void grabdigits(fxdpnt *digi, fxdpnt *a, size_t *gotten, size_t digits_to_get)
{
	memcpy(digi->number, a->number + *gotten, digits_to_get);
        digi->lp += digits_to_get;
        digi->len += digits_to_get;
        *gotten += digits_to_get;

}
fxdpnt *long_sqrt(fxdpnt *a, int base, size_t scale)
{
	a = a;
	base = base;
	scale = scale;
	int odd = 0;
	int digits_to_get = 2;
	fxdpnt *digi = arb_str2fxdpnt("");  //arb_expand(NULL, a->len);
	fxdpnt *g1 = arb_str2fxdpnt("");
	fxdpnt *ans = arb_str2fxdpnt("");
	fxdpnt *fac = arb_str2fxdpnt("");
	fxdpnt *side = arb_str2fxdpnt("");
	fxdpnt *subtract = arb_str2fxdpnt("");
	arb_copy(subtract, a);
	memset(subtract->number, 0, subtract->len);
	

	size_t gotten = 0;
	if (a->lp % 2 == 1)
	{
		odd = 1;
		digits_to_get = 1;
	}
	
	if (odd)
		printf("number was odd -- get 1 digit\n");
	else
		printf("number was even -- get 2 digits\n");

	/* get first set of digits */
	grabdigits(digi, a, &gotten, digits_to_get);
	digits_to_get = 2; 

	printf("digi: ");
	arb_print(digi);
	/* now factorize up to those one or two digits */

	factor2(&fac, digi, base, scale); 
	arb_print(fac);
	pushon(ans, fac);
	arb_print(ans);
	/* now square the ans to get guess 1 */
	arb_mul2(ans, ans, &g1, base, scale); 
	printf("g1 = ");
	arb_print(g1);
	/* now subtract guess 1 from the digi */


	/* now multiply the ans by two into the side */
	arb_mul2(ans, two, &side, base, scale);
	/* now push a zero onto the "side" */
	pushon(side, zero);
	/* now subtract the guess 1 from the original */
	addfront(subtract, g1);
	printf("subtract = ");
	arb_print(subtract);
	arb_sub2(a, subtract, &a, base);
	
	printf("g1 = ");
	arb_print(g1);
	printf("a = ");
	arb_print(a);
	
	printf("bogus ans = ");
	return ans;
}
