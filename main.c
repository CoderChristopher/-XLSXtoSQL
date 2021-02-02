#include <xlsxio_read.h>
#include <mariadb/mysql.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#define MAXCOLUMNLENGTH 1024
#define MAXROWLENGTH 1024
//Global SQL Handle
MYSQL *handle=NULL;
//Both of these structs are dynamic link lists
//Holds data rows
struct datarow{
	char* data;
	struct datarow* next;
};
//Holds data for each titled column
struct datacolumn{
	int items;
	char* title;
	struct datarow* datarows;
	struct datacolumn* next;
};
struct datacolumn* datacolumns=NULL;
//Initialize a column with a title using a provided datacolumn list
void initcolumn(struct datacolumn* dc,char* title){
	if(!dc)
		return;
	dc->items=0;
	dc->title=malloc(MAXCOLUMNLENGTH);
	strcpy(dc->title,title);
	dc->next=NULL;
	return;
}
//Initalizes a row item with a given item of data and data row list
void initrow(struct datarow* dr,char* data){
	if(!dr)
		return;
	dr->data=malloc(MAXROWLENGTH);
	strcpy(dr->data,data);
	dr->next=NULL;
	return;
}
//Adds a new data column with a title
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
//Adds a new data row with a given piece of data given a specific datacolumn to be added to
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
//Finds a column based upon an index where the given index represents the nth item in the dynamic list
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
//Finds a row based upon an index where the given index represents the nth item in the dynamic list
struct datarow* getrowfromindex(int ind,struct datacolumn *dc){
	if(!dc)
		return NULL;
	int i=0;
	struct datarow* dr=dc->datarows;
	while(dr){
		if(ind==i)
			return dr;
		dr=dr->next;
		i++;
	}
	return NULL;
}
//Open a connection to the MYSQL database
void connectsql(){
	handle = mysql_init(NULL);
	mysql_real_connect(handle,"127.0.0.1","copeland","5z2ojgwB","test",3306,NULL,0);
}
//Does a quick check to see if a given character is special (aka non alpha numeric) character
bool isspecial(char cha){
	//Check Ascii ranges...
	if((cha>=0&&cha<48)||(cha>=58&&cha<=64)||(cha>=123&&cha<=127)||(cha>=91&&cha<=96))
		return true;
	return false;
}
//Will take a string and erase the contents between begin and end
ssize_t strerase(char** str,size_t begin,size_t end){
	if(!str)//Make sure the pointer is good
		return -1;
	if(!*str)
		return -1;
	size_t len=strlen(*str);

	//Do some basic sanity checking on the beginning, end, and string length values
	if(len<end)
		return -1;
	if(begin>end)
		return -1;

	//Establish distance as the distance between beginning and end
	size_t dist=end-begin;
	
	//Run through the string
	for(size_t i=begin;i<len;i++){
		//if the distance past the current index is larger then the length of the string then erase by setting to \0
		if((i+dist)>=len)
			(*str)[i]=0;	
		else//Otherwise set the character to the ascii value index + distance
			(*str)[i]=(*str)[i+dist];
	}
}
//Runs through a given string and removes all special character
void cleanstring(char** str){
	//Check to make sure our pointer is good
	if(!str)
		return;
	if(!*str)
		return;
	size_t len=strlen(*str);
	for(size_t i=0;i<len;i++){
		char ch=(*str)[i];//This really is just to make the code look cleaner because typing (*str)[i] is hard and
		//really difficult to intuitively understand. Basically dereference the pointer's pointer, then dereference
		//the string to resolve the char at index i
		if(isspecial(ch)){//If special get that shit outta here
			if(ch=='['||ch=='('||ch=='{'){//If it is a special special character that indicates the opening of
				//a statement then search for the compliment to the opener
				char search=ch;
				char asciioffset=2;//In ascii for [ and { the offset between their complment is 2
				if(search=='(')//But for ( it is 1... that makes sense
					asciioffset=1;
				bool found=false;
				//Do a search through the string looking for the compliment
				for(size_t j=i;j<len;j++){
					if((*str)[j]==search+asciioffset){
						//if found then erase everything inside of the compiments, flag as found 
						//then break out of the for loop
						strerase(str,i,j+1);
						found=true;
						break;
					}
				}
				if(found){
					//So if found, take a step back, becaue we erased everything including whats
					//under the i index, and then charge ahead
					i--;
					continue;
				}
				//Otherwise just remove the [, {, or ( that has no compilment
				strerase(str,i,i);
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
	connectsql();

	char* value;
	xlsxioreadersheet sheet;
	const char* sheetname = NULL;
	xlsxioreadersheetlist sl=xlsxioread_sheetlist_open(xlsxioread);
	//open the first sheet
	if((sheet = xlsxioread_sheet_open(xlsxioread, xlsxioread_sheetlist_next(sl), XLSXIOREAD_SKIP_EMPTY_ROWS)) != NULL){
		//Read the first row
		if(xlsxioread_sheet_next_row(sheet)){
			char querytext[1024];
			memset(querytext,0,1024);
			sprintf(querytext,"create table test.test2(");
			//Read all the titles of the row and start constructing an sql query to create the associated table
			while((value = xlsxioread_sheet_next_cell(sheet))!= NULL){
				cleanstring(&value);
				newdatacolumn(value);
				sprintf(querytext,"%s %s varchar(60),",querytext,value);
				xlsxioread_free(value);
			}
			//erase the final unneeded comma then put a close ); on the end of the statement, run it and
			//check for errors
			querytext[strlen(querytext)-1]=0;
			sprintf(querytext,"%s);",querytext);
			if(mysql_query(handle,querytext)){
				printf("There was an issue with the create query!\n");
				printf("\t*%s\n",mysql_error(handle));
				xlsxioread_sheetlist_close(sl);
				xlsxioread_close(xlsxioread);
				goto clean;
			}

			//Now cycle through all the 'data' rows and read in the data into the sql database
			while (xlsxioread_sheet_next_row(sheet)){
				//clear the memory of the string so we start with a clean slate
				memset(querytext,0,1024);
				//Start constructing the MYSQL query
				strcpy(querytext,"insert into test.test2 set");
				size_t j=0;
				//Read through each cell in row to get its data
				while((value = xlsxioread_sheet_next_cell(sheet))!= NULL){
					//Get the column title for a given cell so that we can set the correct column in
					//SQL
					struct datacolumn* dc=getcolumnfromindex(j,datacolumns);
					if(!dc)
						break;
					sprintf(querytext,"%s %s='%s',",querytext,dc->title,value);
					xlsxioread_free(value);
					j++;
				}
				//Again clear out the last unneeded comma and then drop a ; on the end of the MYSQL query and run it
				querytext[strlen(querytext)-1]=0;
				sprintf(querytext,"%s;",querytext);
				if(mysql_query(handle,querytext)){
					printf("There was an issue with the insert query!\n");
					printf("\t*%s\n",mysql_error(handle));
					goto clean;
				}
			}
			xlsxioread_sheet_close(sheet);
		}
	}
	xlsxioread_sheetlist_close(sl);
	xlsxioread_close(xlsxioread);

	//Real quick print out the new column titles to the console before quitting
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

	clean:
	mysql_close(handle);

	return 0;
}
