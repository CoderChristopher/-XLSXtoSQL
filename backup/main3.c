#include <xlsxio_read.h>
#include <mariadb/mysql.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#define MAXCOLUMNLENGTH 1024
#define MAXROWLENGTH 1024
struct datarow{
	char* data;
	struct datarow* next;
};
struct datacolumn{
	int items;
	char* title;
	struct datarow* datarows;
	struct datacolumn* next;
};
struct datarow{
	char** column;
	size_t elements;
};
struct dataset{
	struct datarow* headers;
	struct datarow** data;
};
struct datacolumn* datacolumns=NULL;
void initcolumn(struct datacolumn* dc,char* title){
	if(!dc)
		return;
	dc->items=0;
	dc->title=malloc(MAXCOLUMNLENGTH);
	strcpy(dc->title,title);
	dc->next=NULL;
	return;
}
void initrow(struct datarow* dr,char* data){
	if(!dr)
		return;
	dr->data=malloc(MAXROWLENGTH);
	strcpy(dr->data,data);
	dr->next=NULL;
	return;
}
void newdatacolumn(char* title){
	if(!datacolumns){
		datacolumns=malloc(sizeof(struct datacolumn));	
		initcolumn(datacolumns,title);
		return;
	}
	struct datacolumn* cur=datacolumns;
	struct datacolumn* prev=NULL;
	while(cur){
		prev=cur;
		cur=cur->next;
	}
	cur=prev->next=malloc(sizeof(struct datacolumn));
	initcolumn(cur,title);	
	return;	
}
void newdatarow(struct datacolumn* dc,char* data){
	if(!dc){
		return;
	}
	struct datarow* cur=dc->datarows;
	if(!cur){
		cur=dc->datarows=malloc(sizeof(struct datarow));
		initrow(cur,data);
		return;
	}
	while(cur->next){
		cur=cur->next;
	}
	cur->next=malloc(sizeof(struct datarow));
	initrow(cur->next,data);
	return;
}
struct datacolumn* getcolumnfromindex(int ind,struct datacolumn *dc){
	if(!dc)
		return NULL;
	int i=0;
	while(dc){
		if(ind==i)
			return dc;
		dc=dc->next;
		i++;
	}
	return NULL;
}
MYSQL *handle=NULL;
void connectsql(){
	handle = mysql_init(NULL);
	mysql_real_connect(handle,"127.0.0.1","copeland","5z2ojgwB","test",3306,NULL,0);
}
bool isspecial(char cha){
	if((cha>=0&&cha<48)||(cha>=58&&cha<=64)||(cha>=123&&cha<=127)||(cha>=91&&cha<=96))
		return true;
	return false;
}
ssize_t strerase(char** str,size_t begin,size_t end){
	if(!str)
		return -1;
	size_t len=strlen(*str);
	if(len<end)
		return -1;
	if(begin>end)
		return -1;
	size_t dist=end-begin;
	for(size_t i=begin;i<len;i++){
		if((i+dist)>=len)
			(*str)[i]=0;	
		else
			(*str)[i]=(*str)[i+dist];
	}
}
void cleanstring(char** str){
	if(!str)
		return;
	if(!*str)
		return;
	size_t len=strlen(*str);
	for(size_t i=0;i<len;i++){
		char ch=(*str)[i];
		if(isspecial(ch)){
			if(ch=='['||ch=='('||ch=='{'){
				char search=ch;
				char asciioffset=2;
				if(search=='(')
					asciioffset=1;
				bool found=false;
				for(size_t j=i;j<len;j++){
					if((*str)[j]==search+asciioffset){
						strerase(str,i,j+1);
						found=true;
						break;
					}
				}
				if(found){
					i=0;
					continue;
				}
			}else{
				strerase(str,i,i);
			}
			
		}
	}

}
int main(int argc,char* argv[]){
	xlsxioreader xlsxioread;
	if((xlsxioread = xlsxioread_open("xtra.xlsx"))== NULL){
		printf("Error opening the file\n");
		return 1; 
	}

	char* value;
	xlsxioreadersheet sheet;
	const char* sheetname = NULL;
	xlsxioreadersheetlist sl=xlsxioread_sheetlist_open(xlsxioread);
	if((sheet = xlsxioread_sheet_open(xlsxioread, xlsxioread_sheetlist_next(sl), XLSXIOREAD_SKIP_EMPTY_ROWS)) != NULL){
		if(xlsxioread_sheet_next_row(sheet)){
			while((value = xlsxioread_sheet_next_cell(sheet))!= NULL){
				cleanstring(&value);
				newdatacolumn(value);
				xlsxioread_free(value);
			}
			while (xlsxioread_sheet_next_row(sheet)){
				size_t i=0;
				while((value = xlsxioread_sheet_next_cell(sheet))!= NULL){
					newdatarow(getcolumnfromindex(i,datacolumns),value);
					xlsxioread_free(value);
					i++;
				}
			}
		}
		printf("\n");
		xlsxioread_sheet_close(sheet);
	}
	xlsxioread_sheetlist_close(sl);
	xlsxioread_close(xlsxioread);

	struct datacolumn* dc=datacolumns;
	while(dc){
		struct datarow* dr=dc->datarows;
		printf("%s\n",dc->title);
		while(dr){
			printf("\t%s\n",dr->data);
			dr=dr->next;
		}
		dc=dc->next;
	}

	connectsql();
	char querytext[1024];
	memset(querytext,0,1024);
	dc=datacolumns;
	sprintf(querytext,"create table test.test2(%s varchar(60)",dc->title);
	while(dc->next){
		dc=dc->next;
		sprintf(querytext,"%s,%s varchar(60)",querytext,dc->title);
	}
	sprintf(querytext,"%s);",querytext);
	printf("%s\n",querytext);
	if(mysql_query(handle,querytext)){
		printf("There was an issue with the query!\n");
		printf("\t*%s\n",mysql_error(handle));
		goto clean;
	}

	int i=0;
	dc=datacolumns;
	while((struct datacolumn* dc1=getdatacolumnfromindex(i,datacolumns))){
		memset(querytext,0,1024);
		sprintf(querytext,"insert into test.test2 set ");
			
	}
	
	strcpy(querytext,"insert into test.test2 set POSITION='A1',DESCRIPTION='this is a desc';");
	if(mysql_query(handle,querytext)){
		printf("There was an issue with the query!\n");
		printf("\t*%s\n",mysql_error(handle));
		goto clean;
	}

	clean:
	mysql_close(handle);

	return 0;
}
