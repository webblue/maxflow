#include <stddef.h>

#ifndef TABLEFIXED_INCLUDED
#define TABLEFIXED_INCLUDED

typedef struct TableFixed *TableFixed_T;
typedef struct TableFixedIter *TableFixedIter_T;



/***************************************************************************/
/*                         TableFixed functions                            */
/***************************************************************************/


TableFixed_T TableFixed_new(unsigned long ulEstLength, size_t uiKeySize);
/* returns a new TableFixed_T. ulEstLength is an estimated max length of
   the table, and each key must contain uiKeySize bytes */

void TableFixed_free(TableFixed_T oTableFixed);
/* frees all memory associated with oTableFixed */

unsigned long TableFixed_length(TableFixed_T oTableFixed);
/* returns the number of bindings in oTableFixed */

int TableFixed_put(TableFixed_T oTableFixed, const void *pvKey, void *pvValue);
/* adds a binding to oTableFixed with the specified pvKey and pvValue and
   returns 1 if successful. rejects bindings with duplicate keys and returns
   0 */

void *TableFixed_getValue(TableFixed_T oTableFixed, const void *pvKey);
/* returns a pointer to the value of the binding with key pvKey. returns NULL
   if no such binding exists */

const void *TableFixed_getKey(TableFixed_T oTableFixed, const void *pvKey);
/* returns a pointer to the key of the binding with key pvKey. returns NULL
   if no such binding exists */

int TableFixed_remove(TableFixed_T oTableFixed, const void *pvKey);
/* removes binding from oTableFixed with a key of pvKey. returns 1 if
   successful and 0 otherwise (such as if no such key exists) */

void TableFixed_toArrays(TableFixed_T oTableFixed, const void **ppvKeyArray,
			 void **ppvValueArray);
/* fills ppvKeyArray with oTableFixed's keys and ppvValueArray with
   oTableFixed's values. the arrays must have enough space allocated for all
   the bindings (at least TableFixed_length() elements) */

void TableFixed_map(TableFixed_T oTableFixed,
		    void (*pfApply)(const void *pvKey, void **ppvValue,
				    void *pvExtra),
		    void *pvExtra);
/* applies function *pfApply to each binding in oTableFixed */



/***************************************************************************/
/*                        TableFixedIter functions                         */
/***************************************************************************/


TableFixedIter_T TableFixedIter_new(TableFixed_T oTableFixed);
/* returns a new TableFixedIter_T, which is initially in an invalid state */

void TableFixedIter_free(TableFixedIter_T oTableFixedIter);
/* frees all dynamically allocated memory associated with oTableFixedIter */

int TableFixedIter_valid(TableFixedIter_T oTableFixedIter);
/* returns 1 if oTableFixedIter is in a valid state, 0 otherwise */

void TableFixedIter_selectFirst(TableFixedIter_T oTableFixedIter);
/* sets the first binding to the current binding and sets oTableFixedIter
   to a valid state if and only if the table contains at least one binding */

void TableFixedIter_selectNext(TableFixedIter_T oTableFixedIter);
/* sets the next binding to the current binding. sets oTableFixedIter to an
   invalid state if there is no next binding */

const void *TableFixedIter_selectedKey(TableFixedIter_T oTableFixedIter);
/* returns a pointer to the current binding's key */

void *TableFixedIter_selectedValue(TableFixedIter_T oTableFixedIter);
/* returns a pointer to the current binding's value */



#endif /* TABLEFIXED_INCLUDED */
