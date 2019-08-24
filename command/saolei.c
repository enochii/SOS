#include<stdio.h> 

#define SIZE 3

char scane[SIZE][SIZE]={'*','*','*','*','*','*','*','*','*'};
int grade[SIZE][SIZE]={50,10,50,10,100,10,50,10,50};

void AI_input(int x,int y);
void display();
int check();
int main_tic()
{
	printf("Welcome Tictactoe, input q to quit game\n");
	int result=0;
	display();
	for(int i=0;i<5;i++)
	{
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
			if(x>0&&x<4&&y>0&&y<4&&(grade[x][y]%10==0)) break;
			else {
				printf("Error,input again:");
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
		scane[x-1][y-1]='o';
		grade[x-1][y-1]=4;
		
		if(check()==1){
			printf("You win\n");
			result=1;
			break;
		}
	
		if(i!=4) AI_input(x-1,y-1);
		
		if(check()==2){
			printf("You lose\n");
			result=1;
			break;
		}
		display();
	}
	if(result==0) printf("Equal\n");
	return 0;
}

void display()
{
	for(int i=0;i<SIZE;i++)
	{
		for(int j=0;j<SIZE;j++)
		{
			printf("%c ",scane[i][j]);
		}
		printf("\n");
	}
}

void AI_input(int x,int y)
{	
	int grade_sum[8];
	grade_sum[0]=grade[0][0]+grade[0][1]+grade[0][2];
	grade_sum[1]=grade[1][0]+grade[1][1]+grade[1][2];
	grade_sum[2]=grade[2][0]+grade[2][1]+grade[2][2];
	grade_sum[3]=grade[0][0]+grade[1][0]+grade[2][0];
	grade_sum[4]=grade[0][1]+grade[1][1]+grade[2][1];
	grade_sum[5]=grade[0][2]+grade[1][2]+grade[2][2];
	grade_sum[6]=grade[0][0]+grade[1][1]+grade[2][2];
	grade_sum[7]=grade[0][2]+grade[1][1]+grade[2][0];
	for(int i=0;i<8;i++){
		if(grade_sum[i]%10==8){
			if(i==0){
				if(grade[0][0]%10==0){
					grade[0][0]=2;
					scane[0][0]='x';
				}
				else if(grade[0][1]%10==0){
					grade[0][1]=2;
					scane[0][1]='x';
				}
				else if(grade[0][2]%10==0){
					grade[0][2]=2;
					scane[0][2]='x';
				}
			}
			else if(i==1){
				if(grade[1][0]%10==0){
					grade[1][0]=1;
					scane[1][0]='x';
				}
				else if(grade[1][1]%10==0){
					grade[1][1]=1;
					scane[1][1]='x';
				}
				else if(grade[1][2]%10==0){
					grade[1][2]=1;
					scane[1][2]='x';
				}
			}
			else if(i==2){
				if(grade[2][0]%10==0){
					grade[2][0]=1;
					scane[2][0]='x';
				}
				else if(grade[2][1]%10==0){
					grade[2][1]=1;
					scane[2][1]='x';
				}
				else if(grade[2][2]%10==0){
					grade[2][2]=1;
					scane[2][2]='x';
				}
			}
			else if(i==3){
				if(grade[0][0]%10==0){
					grade[0][0]=1;
					scane[0][0]='x';
				}
				else if(grade[1][0]%10==0){
					grade[1][0]=1;
					scane[1][0]='x';
				}
				else if(grade[2][0]%10==0){
					grade[2][0]=1;
					scane[2][0]='x';
				}
			}
			else if(i==4){
				if(grade[0][1]%10==0){
					grade[0][1]=1;
					scane[0][1]='x';
				}
				else if(grade[1][1]%10==0){
					grade[1][1]=1;
					scane[1][1]='x';
				}
				else if(grade[2][1]%10==0){
					grade[2][1]=1;
					scane[2][1]='x';
				}
			}
			else if(i==5){
				if(grade[0][2]%10==0){
					grade[0][2]=1;
					scane[0][2]='x';
				}
				else if(grade[1][2]%10==0){
					grade[1][2]=1;
					scane[1][2]='x';
				}
				else if(grade[2][2]%10==0){
					grade[2][2]=1;
					scane[2][2]='x';
				}
			}
			else if(i==6){
				if(grade[0][0]%10==0){
					grade[0][0]=1;
					scane[0][0]='x';
				}
				else if(grade[1][1]%10==0){
					grade[1][1]=1;
					scane[1][1]='x';
				}
				else if(grade[2][2]%10==0){
					grade[2][2]=1;
					scane[2][2]='x';
				}
			}
			else if(i==7){
				if(grade[0][2]%10==0){
					grade[0][2]=1;
					scane[0][2]='x';
				}
				else if(grade[1][1]%10==0){
					grade[1][1]=1;
					scane[1][1]='x';
				}
				else if(grade[2][0]%10==0){
					grade[2][0]=1;
					scane[2][0]='x';
				}
			}
			return;
		}
	}
	
	int max=0;
	int max_x;
	int max_y;
	for(int i=0;i<SIZE;i++)
	{
		for(int j=0;j<SIZE;j++)
		{
			if(grade[i][j]>max){
				max=grade[i][j];
				max_x=i;
				max_y=j;
			} 
		}
		
	}
	grade[max_x][max_y]=1;
	scane[max_x][max_y]='x';
}

int check()
{
	int grade_sum[8];
	grade_sum[0]=grade[0][0]+grade[0][1]+grade[0][2];
	grade_sum[1]=grade[1][0]+grade[1][1]+grade[1][2];
	grade_sum[2]=grade[2][0]+grade[2][1]+grade[2][2];
	grade_sum[3]=grade[0][0]+grade[1][0]+grade[2][0];
	grade_sum[4]=grade[0][1]+grade[1][1]+grade[2][1];
	grade_sum[5]=grade[0][2]+grade[1][2]+grade[2][2];
	grade_sum[6]=grade[0][0]+grade[1][1]+grade[2][2];
	grade_sum[7]=grade[0][2]+grade[1][1]+grade[2][0];
	for(int i=0;i<8;i++)
	{
		if(grade_sum[i]==12) return 1;
		else if(grade_sum[i]==3) return 2;
		else return 0;
	}
}