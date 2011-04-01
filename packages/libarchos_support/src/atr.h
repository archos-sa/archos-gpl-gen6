#ifndef ATR_H
#define ATR_H


namespace archos {

int  atr_load( const char *filepath );
void atr_unload();

const char *atr( const char *key );

}


#endif // ATR_H
