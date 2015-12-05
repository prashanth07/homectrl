#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void print_time (struct tm *p)
{
	printf("sec=%u\tmin=%u\thour=%u\tmday=%u\tmon=%u\tyear=%u\twdat=%u\tyday=%u\tisdst%u\t",
		p->tm_sec,p->tm_min,p->tm_hour,p->tm_mday,p->tm_mon,p->tm_year,p->tm_wday,p->tm_yday,p->tm_isdst);
}

int main()
{
	struct tm tmp;
	char *dir;
	
	time_t t;
	char a[20];
	
	dir = getenv("HOME");
	printf ("Enter day hour minute second %s\n",dir);
	//scanf ("%s",a);
	//t = time(NULL);
	
	//strptime(a,"%w:%H:%M:%S %s",&tmp);
	//print_time(&tmp);
	
	return 0;
}

