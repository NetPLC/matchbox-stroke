#include "matchbox-stroke.h"

/* v.simple hash implementation */

#define HASHSIZE 101

struct UtilHash 
{
  UtilHashNode **hashtab;
  int            size;
};

struct UtilHashNode 
{
   UtilHashNode  *next;
   char          *key;
   pointer       *value;
};

UtilHash*
util_hash_new(void)
{
  UtilHash *hash;

  hash          = util_malloc0(sizeof(UtilHash));
  hash->size    = HASHSIZE;
  hash->hashtab = util_malloc0(sizeof(UtilHashNode)*HASHSIZE);

  return hash;
}

static unsigned int 
hashfunc(UtilHash *hash, char *s)
{
  unsigned int hash_val;

  for(hash_val = 0; *s != '\0'; s++)
    hash_val = *s + 21 * hash_val;

  return hash_val % hash->size;
}

UtilHashNode*
util_hash_lookup_node(UtilHash *hash, char *key)
{
   UtilHashNode *node;

   for (node = hash->hashtab[hashfunc(hash, key)]; 
	node != NULL; 
	node = node->next)
     {
       if (streq(key, node->key))
	 return node;
     }

   return NULL;
}

pointer
util_hash_lookup(UtilHash *hash, char *key)
{
  UtilHashNode *node = NULL;

  node = util_hash_lookup_node(hash, key);

  if (node) 
    return node->value;

  return NULL;
}

void
util_hash_insert(UtilHash *hash, char *key, pointer val)
{
   UtilHashNode *node;
   unsigned int  hashval;

   if ((node = util_hash_lookup_node(hash, key)) == NULL)
     {
       hashval = hashfunc(hash, key);
       
       node       = util_malloc0(sizeof(UtilHashNode));
       node->key  = strdup(key);

       /* link nodes, if any */
       node->next = hash->hashtab[hashval];
       hash->hashtab[hashval] = node;
     } 
   else  /* hmm, key already exists, reset val   */
     free(node->value);
   
   node->value = val;

}

void 
util_hash_destroy(UtilHash *hash)
{
  UtilHashNode *node = NULL, *onode = NULL;
  int           i;

  for (i=0; i<hash->size; i++)
    if (hash->hashtab[i])
      {
	node = hash->hashtab[i];
	while (node != NULL) 
	  {
	    onode = node->next;
	    if (node->key)   free(node->key);
	    /* XXX should call destroyFunc */
	    if (node->value) free(node->value);
	    free(node);
	    node = onode;
	  }
      }

  free(hash->hashtab);
  free(hash);
}
