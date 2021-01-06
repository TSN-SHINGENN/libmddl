/**
 *	Copyright TSN・SINGENN
 *	Basic Author: Seiichi Takeda  '2000-March-01 Active
 *		Last Alteration $Author: takeda $
 */

/**
 * @file smal_dbms.h
 * @brief デバッグメッセージ表示用定義
 */

#ifndef INC_DBMS_H
#define INC_DBMS_H

#include <stdio.h>

#define PRINTF(...) fprintf( stderr, __VA_ARGS__ );

#define DMSG( ...) { PRINTF( __VA_ARGS__ ); }
#define DBMS( ...) { if(1) { PRINTF( __VA_ARGS__ ); }}
#define DBMS1( ...) { if(debuglevel>=1) { PRINTF( __VA_ARGS__ ); }}
#define DBMS2( ...) { if(debuglevel>=2) { PRINTF( __VA_ARGS__ ); }}
#define DBMS3( ...) { if(debuglevel>=3) { PRINTF( __VA_ARGS__ ); }}
#define DBMS4( ...) { if(debuglevel>=4) { PRINTF( __VA_ARGS__ ); }}
#define DBMS5( ...) { if(debuglevel>=5) { PRINTF( __VA_ARGS__ ); }}

#define IFDBG1THEN if(debuglevel>=1)
#define IFDBG2THEN if(debuglevel>=2)
#define IFDBG3THEN if(debuglevel>=3)
#define IFDBG4THEN if(debuglevel>=4)
#define IFDBG5THEN if(debuglevel>=5)

#endif /* end of INC_DBMS_H */
