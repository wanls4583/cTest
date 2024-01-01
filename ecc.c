#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "huge.h"
#include "ecc.h"
#include "hex.h"

void add_points( point *p1, point *p2, huge *p )
{
  point p3;
  huge denominator;
  huge numerator;
  huge invdenom;
  huge lambda;

  set_huge( &denominator, 0 ); 
  copy_huge( &denominator, &p2->x );    // denominator = x2
  subtract( &denominator, &p1->x );     // denominator = x2 - x1
  set_huge( &numerator, 0 );
  copy_huge( &numerator, &p2->y );      // numerator = y2
  subtract( &numerator, &p1->y );       // numerator = y2 - y1
  set_huge( &invdenom, 0 );
  copy_huge( &invdenom, &denominator );
  inv( &invdenom, p );
  set_huge( &lambda, 0 );
  copy_huge( &lambda, &numerator );
  multiply( &lambda, &invdenom );       // lambda = numerator / denominator
  set_huge( &p3.x, 0 );
  copy_huge( &p3.x, &lambda );    // x3 = lambda
  multiply( &p3.x, &lambda );     // x3 = lambda * lambda
  subtract( &p3.x, &p1->x );      // x3 = ( lambda * lambda ) - x1
  subtract( &p3.x, &p2->x );      // x3 = ( lambda * lambda ) - x1 - x2

  divide( &p3.x, p, NULL );       // x3 = ( ( lamdba * lambda ) - x1 - x2 ) % p

  // positive remainder always
  if ( p3.x.sign ) 
  {
    p3.x.sign = 0;
    subtract( &p3.x, p );
    p3.x.sign = 0;
  }

  set_huge( &p3.y, 0 );
  copy_huge( &p3.y, &p1->x );    // y3 = x1
  subtract( &p3.y, &p3.x );      // y3 = x1 - x3
  multiply( &p3.y, &lambda );    // y3 = ( x1 - x3 ) * lambda
  subtract( &p3.y, &p1->y );     // y3 = ( ( x1 - x3 ) * lambda ) - y

  divide( &p3.y, p, NULL );
  // positive remainder always
  if ( p3.y.sign )
  {
    p3.y.sign = 0;
    subtract( &p3.y, p );
    p3.y.sign = 0;
  }

  // p1->x = p3.x
  // p1->y = p3.y
  copy_huge( &p1->x, &p3.x );
  copy_huge( &p1->y, &p3.y );

  free_huge( &p3.x );
  free_huge( &p3.y );
  free_huge( &denominator );
  free_huge( &numerator );
  free_huge( &invdenom );
  free_huge( &lambda );
}

static void double_point( point *p1, huge *a, huge *p )
{
  huge lambda;
  huge l1;
  huge x1;
  huge y1;

  set_huge( &lambda, 0 );
  set_huge( &x1, 0 );
  set_huge( &y1, 0 );
  set_huge( &lambda, 2 );     // lambda = 2;
  multiply( &lambda, &p1->y );  // lambda = 2 * y1
  inv( &lambda, p );       // lambda = ( 2 * y1 ) ^ -1 (% p)

  set_huge( &l1, 3 );       // l1 = 3
  multiply( &l1, &p1->x );    // l1 = 3 * x
  multiply( &l1, &p1->x );    // l1 = 3 * x ^ 2
  add( &l1, a );         // l1 = ( 3 * x ^ 2 ) + a
  multiply( &lambda, &l1 );    // lambda = [ ( 3 * x ^ 2 ) + a ] / [ 2 * y1 ] ) % p
  copy_huge( &y1, &p1->y );
  // Note - make two copies of x2; this one is for y1 below
  copy_huge( &p1->y, &p1->x );
  set_huge( &x1, 2 );
  multiply( &x1, &p1->x );    // x1 = 2 * x1

  copy_huge( &p1->x, &lambda );  // x1 = lambda
  multiply( &p1->x, &lambda );  // x1 = ( lambda ^ 2 );
  subtract( &p1->x, &x1 );    // x1 = ( lambda ^ 2 ) - ( 2 * x1 )
  divide( &p1->x, p, NULL );   // [ x1 = ( lambda ^ 2 ) - ( 2 * x1 ) ] % p
  
  if ( p1->x.sign )
  {
    subtract( &p1->x, p );
    p1->x.sign = 0;
    subtract( &p1->x, p );
  }
  subtract( &p1->y, &p1->x );  // y3 = x3 � x1
  multiply( &p1->y, &lambda ); // y3 = lambda * ( x3 - x1 );
  subtract( &p1->y, &y1 );   // y3 = ( lambda * ( x3 - x1 ) ) - y1
  divide( &p1->y, p, NULL );  // y3 = [ ( lambda * ( x3 - x1 ) ) - y1 ] % p
  if ( p1->y.sign )
  {
    p1->y.sign = 0;
    subtract( &p1->y, p );
    p1->y.sign = 0;
  }

  free_huge( &lambda );
  free_huge( &x1 );
  free_huge( &y1 );
  free_huge( &l1 );
}

void multiply_point( point *p1, huge *k, huge *a, huge *p )
{
  int i;
  unsigned char mask;
  point dp;
  int paf = 1;

  set_huge( &dp.x, 0 );
  set_huge( &dp.y, 0 );
  copy_huge( &dp.x, &p1->x );
  copy_huge( &dp.y, &p1->y );
  for ( i = k->size; i; i-- )
  {
    for ( mask = 0x01; mask; mask <<= 1 )
    {
      if ( k->rep[ i - 1 ] & mask )
      {
       if ( paf )
       {
         paf = 0;
         copy_huge( &p1->x, &dp.x );
         copy_huge( &p1->y, &dp.y );
       }
       else
       {
         add_points( p1, &dp, p );
        //  printf("before-----------:\n");
        //  show_hex(p1->x.rep, p1->x.size);
        //  show_hex(p1->y.rep, p1->y.size);
        //  printf("double:\n");
        //  show_hex(dp.x.rep, dp.x.size);
        //  show_hex(dp.y.rep, dp.y.size);
        //  add_points(p1, &dp, p);
        //  printf("after-----------:\n");
        //  show_hex(p1->x.rep, p1->x.size);
        //  show_hex(p1->y.rep, p1->y.size);
       }
     }
     // double dp
     double_point( &dp, a, p );
    //  printf("double:\n");
    //  show_hex(dp.x.rep, dp.x.size);
    //  show_hex(dp.y.rep, dp.y.size);
    }
  } 

  free_huge( &dp.x );
  free_huge( &dp.y );
}

int main() {
    clock_t start, end;
    int _a = 1, b = 1, _p = 23;
    point p1, p2;
    huge a, p, k;
    set_huge(&a, _a);
    set_huge(&p, _p);

    // for (int x = 0; x < 100; x += 1) {
    //     // if (x != 90) {
    //     //   continue;
    //     // }
    //     int y = x * x * x + _a * x * x + b, r = y * 2 % _p;
    //     printf("x=%d,y=%d,r=%d\n", x, y, r);
    //     set_huge(&p1.x, x);
    //     set_huge(&p1.y, y);
    //     double_point(&p1, &a, &p);
    //     show_hex(p1.x.rep, p1.x.size);
    //     show_hex(p1.y.rep, p1.y.size);
    // }

    // set_huge(&p1.x, 1);
    // set_huge(&p1.y, 0);
    // double_point(&p1, &a, &p);
    // show_hex(p1.x.rep, p1.x.size);
    // show_hex(p1.y.rep, p1.y.size);

    // for (int x = 0; x < 200; x += 2) {
    //     int x1 = x, x2 = x+1;
    //     int y1 = x1 * x1 * x1 + _a * x1 * x1 + b;
    //     int y2 = x2 * x2 * x2 + _a * x2 * x2 + b;
    //     printf("x1=%d,y1=%d,x2=%d,y2=%d\n", x1, y1, x2, y2);
    //     set_huge(&p1.x, x1);
    //     set_huge(&p1.y, y1);
    //     set_huge(&p2.x, x2);
    //     set_huge(&p2.y, y2);
    //     add_points(&p1, &p2, &p);
    //     show_hex(p1.x.rep, p1.x.size);
    //     show_hex(p1.y.rep, p1.y.size);
    // }

    // set_huge(&p1.x, 0x04);
    // set_huge(&p1.y, 0x51);
    // set_huge(&p2.x, 1);
    // set_huge(&p2.y, 3);
    // add_points(&p1, &p2, &p);
    // show_hex(p1.x.rep, p1.x.size);
    // show_hex(p1.y.rep, p1.y.size);
    start = clock();
    for (int x = 0; x < 1000; x += 1) {
        // if (x != 126) {
        //     continue;
        // }
        int y = x * x * x + _a * x * x + b, r = y * 2 % _p;
        printf("x=%d,y=%d,r=%d\n", x, y, r);
        set_huge(&p1.x, x);
        set_huge(&p1.y, y);
        set_huge(&k, 1234);
        multiply_point(&p1, &k, &a, &p);
        show_hex(p1.x.rep, p1.x.size);
        show_hex(p1.y.rep, p1.y.size);

        set_huge(&p1.x, x);
        set_huge(&p1.y, y);
        set_huge(&k, 101);
        multiply_point(&p1, &k, &a, &p);
        show_hex(p1.x.rep, p1.x.size);
        show_hex(p1.y.rep, p1.y.size);
    }
    end = clock();
    printf("duration: %fs", (double)(end - start) / CLOCKS_PER_SEC);
    
    return 0;
}