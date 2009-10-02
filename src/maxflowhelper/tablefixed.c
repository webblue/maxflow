#include "tablefixed.h"
#include <stdlib.h>
#include <limits.h>
#include <assert.h>


/***************************************************************************/
/*                           Data structures                               */
/***************************************************************************/


struct TableFixed {
  unsigned long ulNumBindings;
  size_t uiKeySize;
  unsigned long ulNumBuckets;
  int (*compare)(const void *, const void *, size_t);
  unsigned long (*hash)(const void *, size_t);
  struct Node **ppnArray;
};

struct Node {
  const void *pvKey;
  void *pvValue;
  struct Node *next;
};

struct TableFixedIter {
  unsigned long ulCurrentBucket;
  struct TableFixed *oTableFixed;
  struct Node *pnCurrentNode;
};

typedef struct Node *Node_L;


/***************************************************************************/
/*                      Local function declarations                        */
/***************************************************************************/


static Node_L getNode(TableFixed_T oTableFixed, const void *pvKey);
/* returns a pointer to the node with key pvKey, or NULL if there is no
   such node */

static unsigned long calculateBuckets(unsigned long ulEstLength);
/* returns the number of buckets that will be used in the hash table
   based on the estimated length */

/* the following hash functions hash the key and return the hash code. hash8,
   hash 12, and hash16 are optimized for keys of the respective size */
static unsigned long hashGeneric(const void *pvKey, size_t uiKeySize);
static unsigned long hash8(const void *pvKey, size_t uiKeySize);
static unsigned long hash12(const void *pvKey, size_t uiKeySize);
static unsigned long hash16(const void *pvKey, size_t uiKeySize);

/* the following compare functions return 0 if the two keys are equal, and
   1 otherwise. compare8, compare12, and compare16 are optimized for keys
   of the respective sizes */
static int compareGeneric(const void *pvKey1, const void *pvKey2,
			  size_t uiKeySize);
static int compare8(const void *pvKey1, const void *pvKey2, size_t uiKeySize);
static int compare12(const void *pvKey1, const void *pvKey2, size_t uiKeySize);
static int compare16(const void *pvKey1, const void *pvKey2, size_t uiKeySize);


/***************************************************************************/
/*                      TableFixed implementation                          */
/***************************************************************************/


TableFixed_T TableFixed_new(unsigned long ulEstLength, size_t uiKeySize)

/* returns a new TableFixed_T. ulEstLength is an estimated max length of
   the table, and each key must contain uiKeySize bytes */

{
  TableFixed_T oTableFixed;

  oTableFixed = (TableFixed_T) malloc(sizeof(*oTableFixed));
  assert(oTableFixed != NULL);

  /* initialize the fields */
  oTableFixed->ulNumBindings = 0;
  oTableFixed->uiKeySize = uiKeySize;
  oTableFixed->ulNumBuckets = calculateBuckets(ulEstLength);

  /* "install" the appropriate compare and hash functions */
  switch(uiKeySize) {
  case 8: 
    oTableFixed->compare = compare8;
    oTableFixed->hash = hash8;
    break;
  case 12:
    oTableFixed->compare = compare12;
    oTableFixed->hash = hash12;
    break;
  case 16:
    oTableFixed->compare = compare16;
    oTableFixed->hash = hash16;
    break;
  default:
    oTableFixed->compare = compareGeneric;
    oTableFixed->hash = hashGeneric;
  }

  /* allocate memory for the array of pointers to bindings (nodes) */
  oTableFixed->ppnArray = (Node_L *) calloc(oTableFixed->ulNumBuckets,
					    sizeof(*oTableFixed->ppnArray));
  assert(oTableFixed->ppnArray != NULL);

  return oTableFixed;
}


void TableFixed_free(TableFixed_T oTableFixed)

/* frees all memory associated with oTableFixed */

{
  int i;
  Node_L pn1, pn2;

  assert(oTableFixed != NULL);

  /* free the linked lists of bindings (nodes) */
  for(i = 0; i < oTableFixed->ulNumBuckets; i++) {
    pn1 = oTableFixed->ppnArray[i];
    while(pn1 != NULL) {
      pn2 = pn1->next;
      free(pn1);
      pn1 = pn2;
    }
  }

  free(oTableFixed->ppnArray);
  free(oTableFixed);
}


unsigned long TableFixed_length(TableFixed_T oTableFixed)

/* returns the number of bindings in oTableFixed */

{
  assert(oTableFixed != NULL);
  return oTableFixed->ulNumBindings;
}


int TableFixed_put(TableFixed_T oTableFixed, const void *pvKey, void *pvValue)

/* adds a binding to oTableFixed with the specified pvKey and pvValue and
   returns 1 if successful. rejects bindings with duplicate keys and returns
   0 */

{
  unsigned long ulHashCode;
  Node_L *ppnNode;
  Node_L pnNewNode;

  assert(oTableFixed != NULL);
  assert(pvKey != NULL);
  assert(pvValue != NULL);

  /* return 0 if pvKey already exists in oTableFixed */
  if(getNode(oTableFixed, pvKey) != NULL)
    return 0;

  /* find appropriate bucket */
  ulHashCode = (*oTableFixed->hash)(pvKey, oTableFixed->uiKeySize);
  ppnNode = oTableFixed->ppnArray + (ulHashCode % oTableFixed->ulNumBuckets);

  /* create new node */
  pnNewNode = (Node_L) malloc(sizeof(*pnNewNode));
  assert(pnNewNode != NULL);
  pnNewNode->pvKey = pvKey;
  pnNewNode->pvValue = pvValue;
 
  /* link new node into list */
  pnNewNode->next = *ppnNode;
  *ppnNode = pnNewNode;

  oTableFixed->ulNumBindings++;

  return 1;
}


void *TableFixed_getValue(TableFixed_T oTableFixed, const void *pvKey)

/* returns a pointer to the value of the binding with key pvKey. returns NULL
   if no such binding exists */

{
  Node_L pnNode;

  assert(oTableFixed != NULL);
  assert(pvKey);
  
  pnNode = getNode(oTableFixed, pvKey);
  if(pnNode == NULL)
    return NULL;
  return pnNode->pvValue;
}


const void *TableFixed_getKey(TableFixed_T oTableFixed, const void *pvKey)

/* returns a pointer to the key of the binding with key pvKey. returns NULL
   if no such binding exists */

{
  Node_L pnNode;

  assert(oTableFixed != NULL);
  assert(pvKey != NULL);
  
  pnNode = getNode(oTableFixed, pvKey);
  if(pnNode == NULL)
    return NULL;
  return pnNode->pvKey;
}


int TableFixed_remove(TableFixed_T oTableFixed, const void *pvKey)

/* removes binding from oTableFixed with a key of pvKey. returns 1 if
   successful and 0 otherwise (such as if no such key exists) */

{
  Node_L pnNode, pnNodeBefore;
  Node_L *ppnArray;
  size_t uiKeySize;
  unsigned long ulHashCode;

  assert(oTableFixed != NULL);
  assert(pvKey != NULL);

  ppnArray = oTableFixed->ppnArray;
  uiKeySize = oTableFixed->uiKeySize;
  ulHashCode = (*oTableFixed->hash)(pvKey, uiKeySize);
  pnNodeBefore = ppnArray[ulHashCode % oTableFixed->ulNumBuckets];

  /* check if bucket is empty */
  if(pnNodeBefore == NULL)
    return 0;

  /* check if first node of list is to be removed */
  if((*oTableFixed->compare)(pvKey, pnNodeBefore->pvKey, uiKeySize) == 0) {
    ppnArray[ulHashCode % oTableFixed->ulNumBuckets] = pnNodeBefore->next;
    free(pnNodeBefore);
  }

  else {
    /* make pnNodeBefore point to node before the node to be removed */
    while(pnNodeBefore->next != NULL &&
	  (*oTableFixed->compare)(pvKey, pnNodeBefore->next->pvKey,
				  oTableFixed->uiKeySize) == 1)
      pnNodeBefore = pnNodeBefore->next;

    /* check if key is not in table */
    if(pnNodeBefore->next == NULL)
      return 0;

    /* remove binding */
    pnNode = pnNodeBefore->next;
    pnNodeBefore->next = pnNode->next;
    free(pnNode);
  }

  oTableFixed->ulNumBindings--;

  return 1;
}


void TableFixed_toArrays(TableFixed_T oTableFixed, const void **ppvKeyArray,
			 void **ppvValueArray)

/* fills ppvKeyArray with oTableFixed's keys and ppvValueArray with
   oTableFixed's values. the arrays must have enough space allocated for all
   the bindings (at least TableFixed_length() elements) */

{
  Node_L pnNode;
  unsigned long i, ulLength;
  unsigned long ulPosition;

  assert(oTableFixed != NULL);
  assert(ppvKeyArray != NULL);
  assert(ppvValueArray != NULL);

  ulLength = oTableFixed->ulNumBuckets;

  ulPosition = 0;
  for(i = 0; i < ulLength; i++) {
    pnNode = oTableFixed->ppnArray[i];
    while(pnNode != NULL) {
      ppvKeyArray[ulPosition] = pnNode->pvKey;
      ppvValueArray[ulPosition++] = pnNode->pvValue;
      pnNode = pnNode->next;
    }
  }
}


void TableFixed_map(TableFixed_T oTableFixed,
		    void (*pfApply)(const void *pvKey, void **ppvValue,
				    void *pvExtra),
		    void *pvExtra)

/* applies function *pfApply to each binding in oTableFixed */

{
  Node_L pnNode;
  unsigned long i, ulLength;

  assert(oTableFixed != NULL);
  assert(pfApply != NULL);

  ulLength = oTableFixed->ulNumBuckets;

  for(i = 0; i < ulLength; i++) {
    pnNode = oTableFixed->ppnArray[i];
    while(pnNode != NULL) {
      (*pfApply)(pnNode->pvKey, &pnNode->pvValue, pvExtra);
      pnNode = pnNode->next;
    }
  }
}



/***************************************************************************/
/*                      Local function definitions                         */
/***************************************************************************/


static Node_L getNode(TableFixed_T oTableFixed, const void *pvKey)

/* returns a pointer to the node with key pvKey, or NULL if there is no
   such node */

{
  size_t uiKeySize;
  unsigned long ulHashCode;
  Node_L pnNode;

  assert(oTableFixed != NULL);
  assert(pvKey != NULL);

  /* find appropriate bucket */
  uiKeySize = oTableFixed->uiKeySize;
  ulHashCode = (*oTableFixed->hash)(pvKey, uiKeySize);
  pnNode = oTableFixed->ppnArray[ulHashCode % oTableFixed->ulNumBuckets];

  /* find node in bucket (keep pnNode NULL if it does not exist) */
  while(pnNode != NULL &&
	(*oTableFixed->compare)(pvKey, pnNode->pvKey, uiKeySize) == 1)
    pnNode = pnNode->next;

  return pnNode;
}


static unsigned long calculateBuckets(unsigned long ulEstLength)

/* returns the number of buckets that will be used in the hash table
   based on the estimated length */

{
  /* this code taken from Hanson book */

  int i;
  static unsigned long primes[] = { 509, 509, 1021, 2053, 4093, 8191, 16381,
				    32771, 65521, 130003, 260003, 520019,
				    1040021, 2080003, 4160003, 8320001,
				    16000057, 32000011, 64000031, 128000003,
				    256000001, 512000009, 1000000007,
				    1999999973, ULONG_MAX };

  for(i = 1; primes[i] < ulEstLength; i++);
  return primes[i - 1];
}


static unsigned long hashGeneric(const void *pvKey, size_t uiKeySize)

/* hashes pvKey (returns the sum of its bytes) */

{
  unsigned long ulHashCode = 0;
  char *pcKey = (char *) pvKey;
  size_t i;

  assert(pvKey != NULL);

  for(i = 0; i < uiKeySize; i++)
    ulHashCode += (unsigned long) pcKey[i];

  return ulHashCode;
}


static unsigned long hash8(const void *pvKey, size_t uiKeySize)

/* same as hashGeneric, but optimized for keys 8 bytes long */

{
  char *pcKey = (char *) pvKey;

  assert(pvKey != NULL);

  return (unsigned long) pcKey[0] + (unsigned long) pcKey[1] +
    (unsigned long) pcKey[2] + (unsigned long) pcKey[3] +
    (unsigned long) pcKey[4] + (unsigned long) pcKey[5] +
    (unsigned long) pcKey[6] + (unsigned long) pcKey[7];
}


static unsigned long hash12(const void *pvKey, size_t uiKeySize)

/* same as hashGeneric, but optimized for keys 12 bytes long */

{
  char *pcKey = (char *) pvKey;

  assert(pvKey != NULL);

  return (unsigned long) pcKey[0] + (unsigned long) pcKey[1] +
    (unsigned long) pcKey[2] + (unsigned long) pcKey[3] +
    (unsigned long) pcKey[4] + (unsigned long) pcKey[5] +
    (unsigned long) pcKey[6] + (unsigned long) pcKey[7] +
    (unsigned long) pcKey[8] + (unsigned long) pcKey[9] +
    (unsigned long) pcKey[10] + (unsigned long) pcKey[11];
}


static unsigned long hash16(const void *pvKey, size_t uiKeySize)

/* same as hashGeneric, but optimized for keys 16 bytes long */

{
  char *pcKey = (char *) pvKey;

  assert(pvKey != NULL);

  return (unsigned long) pcKey[0] + (unsigned long) pcKey[1] +
    (unsigned long) pcKey[2] + (unsigned long) pcKey[3] +
    (unsigned long) pcKey[4] + (unsigned long) pcKey[5] +
    (unsigned long) pcKey[6] + (unsigned long) pcKey[7] +
    (unsigned long) pcKey[8] + (unsigned long) pcKey[9] +
    (unsigned long) pcKey[10] + (unsigned long) pcKey[11] +
    (unsigned long) pcKey[12] + (unsigned long) pcKey[13] +
    (unsigned long) pcKey[14] + (unsigned long) pcKey[15];
}


static int compareGeneric(const void *pvKey1, const void *pvKey2,
			  size_t uiKeySize)

/* returns 0 if pvKey1 and pvKey2 are equal, 1 otherwise */

{
  char *pcKey1 = (char *) pvKey1, *pcKey2 = (char *) pvKey2;
  size_t i;

  assert(pvKey1 != NULL);
  assert(pvKey2 != NULL);

  for(i = 0; i < uiKeySize; i++)
    if(pcKey1[i] != pcKey2[i])
      return 1;
  return 0;
}


static int compare8(const void *pvKey1, const void *pvKey2, size_t uiKeySize)

/* same as compareGeneric but optimized for keys 8 bytes long */

{
  char *pcKey1 = (char *) pvKey1, *pcKey2 = (char *) pvKey2;

  assert(pvKey1 != NULL);
  assert(pvKey2 != NULL);

  if(pcKey1[0] == pcKey2[0] && pcKey1[1] == pcKey2[1] &&
     pcKey1[2] == pcKey2[2] && pcKey1[3] == pcKey2[3] &&
     pcKey1[4] == pcKey2[4] && pcKey1[5] == pcKey2[5] &&
     pcKey1[6] == pcKey2[6] && pcKey1[7] == pcKey2[7])
    return 0;
  return 1;
}


static int compare12(const void *pvKey1, const void *pvKey2, size_t uiKeySize)

/* same as compareGeneric but optimized for keys 12 bytes long */

{
  char *pcKey1 = (char *) pvKey1, *pcKey2 = (char *) pvKey2;

  assert(pvKey1 != NULL);
  assert(pvKey2 != NULL);

  if(pcKey1[0] == pcKey2[0] && pcKey1[1] == pcKey2[1] &&
     pcKey1[2] == pcKey2[2] && pcKey1[3] == pcKey2[3] &&
     pcKey1[4] == pcKey2[4] && pcKey1[5] == pcKey2[5] &&
     pcKey1[6] == pcKey2[6] && pcKey1[7] == pcKey2[7] &&
     pcKey1[8] == pcKey2[8] && pcKey1[9] == pcKey2[9] &&
     pcKey1[10] == pcKey2[10] && pcKey1[11] == pcKey2[11])
    return 0;
  return 1;
}


static int compare16(const void *pvKey1, const void *pvKey2, size_t uiKeySize)

/* same as compareGeneric but optimized for keys 16 bytes long */

{
  char *pcKey1 = (char *) pvKey1, *pcKey2 = (char *) pvKey2;

  assert(pvKey1 != NULL);
  assert(pvKey2 != NULL);

  if(pcKey1[0] == pcKey2[0] && pcKey1[1] == pcKey2[1] &&
     pcKey1[2] == pcKey2[2] && pcKey1[3] == pcKey2[3] &&
     pcKey1[4] == pcKey2[4] && pcKey1[5] == pcKey2[5] &&
     pcKey1[6] == pcKey2[6] && pcKey1[7] == pcKey2[7] &&
     pcKey1[8] == pcKey2[8] && pcKey1[9] == pcKey2[9] &&
     pcKey1[10] == pcKey2[10] && pcKey1[11] == pcKey2[11] &&
     pcKey1[12] == pcKey2[12] && pcKey1[13] == pcKey2[13] &&
     pcKey1[14] == pcKey2[14] && pcKey1[15] == pcKey2[15])
    return 0;
  return 1;
}



/***************************************************************************/
/*                     TableFixedIter implementation                       */
/***************************************************************************/


TableFixedIter_T TableFixedIter_new(TableFixed_T oTableFixed)

/* returns a new TableFixedIter_T, which is initially in an invalid state */

{
  TableFixedIter_T oTableFixedIter;

  assert(oTableFixed != NULL);

  oTableFixedIter = (TableFixedIter_T) malloc(sizeof(*oTableFixedIter));
  assert(oTableFixedIter != NULL);

  /* initialize fields */
  oTableFixedIter->ulCurrentBucket = 0;
  oTableFixedIter->oTableFixed = oTableFixed;
  oTableFixedIter->pnCurrentNode = NULL;

  return oTableFixedIter;
}


void TableFixedIter_free(TableFixedIter_T oTableFixedIter)

/* frees all dynamically allocated memory associated with oTableFixedIter */

{
  assert(oTableFixedIter != NULL);
  free(oTableFixedIter);
}


int TableFixedIter_valid(TableFixedIter_T oTableFixedIter)

/* returns 1 if oTableFixedIter is in a valid state, 0 otherwise */

{
  assert(oTableFixedIter != NULL);
  if(oTableFixedIter->pnCurrentNode == NULL)
    return 0;
  return 1;
}


void TableFixedIter_selectFirst(TableFixedIter_T oTableFixedIter)

/* sets the first binding to the current binding and sets oTableFixedIter
   to a valid state if and only if the table contains at least one binding */

{
  unsigned long i, ulLength;
  TableFixed_T oTableFixed;

  assert(oTableFixedIter != NULL);

  oTableFixed = oTableFixedIter->oTableFixed;
  ulLength = oTableFixed->ulNumBuckets;
  oTableFixedIter->pnCurrentNode = NULL;

  /* loop through buckets until a non-NULL one is found */
  for(i = 0; i < ulLength; i++) {
    if(oTableFixed->ppnArray[i] != NULL) {
      oTableFixedIter->pnCurrentNode = oTableFixed->ppnArray[i];
      oTableFixedIter->ulCurrentBucket = i;
      return;
    }
  }
}


void TableFixedIter_selectNext(TableFixedIter_T oTableFixedIter)

/* sets the next binding to the current binding. sets oTableFixedIter to an
   invalid state if there is no next binding */

{
  unsigned long i, ulLength;
  TableFixed_T oTableFixed;

  assert(oTableFixedIter != NULL);
  assert(oTableFixedIter->pnCurrentNode != NULL);

  oTableFixed = oTableFixedIter->oTableFixed;

  /* if the next binding is in the same bucket */
  if(oTableFixedIter->pnCurrentNode->next != NULL) {
    oTableFixedIter->pnCurrentNode = oTableFixedIter->pnCurrentNode->next;
    return;
  }

  ulLength = oTableFixed->ulNumBuckets;

  /* otherwise, step through buckets until find a binding */
  for(i = oTableFixedIter->ulCurrentBucket + 1; i < ulLength; i++) {
    if(oTableFixed->ppnArray[i] != NULL) {
      oTableFixedIter->pnCurrentNode = oTableFixed->ppnArray[i];
      oTableFixedIter->ulCurrentBucket = i;
      return;
    }
  }

  /* if there are no bindings, state becomes invalid */
  oTableFixedIter->pnCurrentNode = NULL;
}


const void *TableFixedIter_selectedKey(TableFixedIter_T oTableFixedIter)

/* returns a pointer to the current binding's key */

{
  assert(oTableFixedIter != NULL);
  assert(oTableFixedIter->pnCurrentNode != NULL);
  return oTableFixedIter->pnCurrentNode->pvKey;
}


void *TableFixedIter_selectedValue(TableFixedIter_T oTableFixedIter)

/* returns a pointer to the current binding's value */

{
  assert(oTableFixedIter != NULL);
  assert(oTableFixedIter->pnCurrentNode != NULL);
  return oTableFixedIter->pnCurrentNode->pvValue;
}
