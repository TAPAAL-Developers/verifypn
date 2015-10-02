/* 
 * File:   CTLquery.h
 * Author: mossns
 *
 * Created on September 30, 2015, 11:12 AM
 */

#ifndef CTLQUERY_H
#define	CTLQUERY_H

enum Quantifier { AND, OR, A, E, NEG };
enum Path { G, F, X, U };
enum Atom { IsFireable } ;

struct CTLquery {
  Quantifier quantifier;
  Path path;
  CTLquery tail[];
  
} ;



#endif	/* CTLQUERY_H */

