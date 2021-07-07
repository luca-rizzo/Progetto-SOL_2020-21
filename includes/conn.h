#ifndef CONN_H
#define CONN_H
#include<conn.h>
#include<util.h>
typedef enum{
	cr,
	rd,
	wr,
	op,
	cl,
	rn,
	re,
	ap
}t_op;
#define SOCKNAME "./tmp/csSock"
#define UNIX_PATH_MAX 108
#endif
