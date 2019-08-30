#include <stdio.h>

char map_hidden[9][9]={
'*','*','*','*','*','*','*','*','*',
'*','*','*','*','*','*','*','*','*',
'*','*','*','*','*','*','*','*','*',
'*','*','*','*','*','*','*','*','*',
'*','*','*','*','*','*','*','*','*',
'*','*','*','*','*','*','*','*','*',
'*','*','*','*','*','*','*','*','*',
'*','*','*','*','*','*','*','*','*',
'*','*','*','*','*','*','*','*','*',
};



void show_map();
int win();

int mainboom()
{
	int i,j;
	for(i=0;i<9;i++){
		for(j=0;j<9;j++){
			map_hidden[i][j]='*';
		}
	}
	char map[9][9]={
	'X','2','1','1','1','1','1','X','1',
	'2','X','2','3','X','2','2','2','2',
	'1','2','X','3','X','3','2','X','1',
	'1','2','3','3','3','3','X','3','2',
	'1','X','2','X','3','X','3','3','X',
	'1','1','3','3','X','3','X','2','1',
	'1','2','2','X','3','3','3','3','2',
	'X','3','X','3','3','X','3','X','X',
	'2','X','3','X','2','1','3','X','3',
	};
	printf("Welcome to minesweep!\n");
	show_map();
	
	while(1){
		printf("Please input(x y):");
		char input[4];
		for(int i=0;i<4;i++)
		{
		    input[i]=0;	

		}
		char x,y;
		//scanf("%d,%d",&x,&y);
		int r=read(0,input,4);
		input[r]=0;
		x=input[0]-48;
		y=input[2]-48;
		
		if(x+48=='q'){
		   printf("Game Over\n"); 
		   return 0;
		    
		}
		while(1){
			if(x>0&&x<=9&&y>0&&y<=9&&map_hidden[x-1][y-1]=='*') break;
			else{
				printf("Error,please input again:");
				int r=read(0,input,4);
				input[r]=0;
				x=input[0]-48;
				y=input[2]-48;
		
				if(x+48=='q'){
				   printf("Game Over\n"); 
				   return 0;
				    
				}
			}
		}
		
		int m=x-1;
		int n=y-1;
		
		if(map[m][n]=='X'){
			printf("Game Over!\n");
			return 0;
		}
		int i,j; 
		for(i=m-1;i<x+1;i++){
			if(i>=0&&i<=8){
				for(j=n-1;j<y+1;j++){
					if(j>=0&&j<=8){
						if(map[i][j]!='X')map_hidden[i][j]=map[i][j];
					}
				}
			}
			
		}
		show_map();
		if(win()){
			printf("You Win!\n");
			break;
		}
	}
	return 0;
}

void show_map()
{
	int i,j;
	for(i=0;i<9;i++){
		for(j=0;j<9;j++){
			printf("%c ",map_hidden[i][j]);
		}
		printf("\n");
	}
}

int win(){
	int sum=0;
	int i,j;
	for(i=0;i<9;i++){
		for(j=0;j<9;j++){
			if(map_hidden[i][j]=='*') sum++;
		}
	}
	if(sum==23) return 1;
	else return 0;
}
