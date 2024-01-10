#ifndef wnhashH
#define wnhashH


int wn_strhash(char *key);
int wn_inthash(int i);
int wn_ptrhash(ptr p);
int wn_memhash(ptr key,int len);

void wn_cumhasho(int *paccum,int i);
void wn_cumhashu(int *paccum,int i);


#endif


