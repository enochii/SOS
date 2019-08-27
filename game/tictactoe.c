#include<stdio.h> 

#define SIZE 3

static char scane[SIZE][SIZE]={'*','*','*','*','*','*','*','*','*'};
static int grade[SIZE][SIZE]={50,10,50,10,100,10,50,10,50};

static void AI_input(int x,int y);
static void display();
static int check();

static void clear()
{
    for(int i=0;i<SIZE;i++)
	{
		for(int j=0;j<SIZE;j++)
		{
			scane[i][j]='*';
		}
	}
}

int main_tic()
{
    clear();
	// printf("done\n");
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
			if(x>0&&x<4&&y>0&&y<4&&(scane[x-1][y-1]=='*')) break;
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
		
        int res=check();
		if(res==1){
			printf("You win\n");
			result=1;
			break;
		}else if(res==2){
            printf("Equal\n");
            break;
        }
	
		if(i!=4) AI_input(x-1,y-1);
        res=check();
		
		if(res==0){
			printf("You lose\n");
			result=1;
			break;
		}
        
		display();
	}
	// if(result==0) printf("Equal\n");
	return 0;
}

static void display()
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

static void AI_input(int x,int y)
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

static int check()
{
    // printf("!!!");
    char(*mat)[3]=scane;
    int flag=0;
    const char o='o';
    const char star='*';
    //per row
    for(int i=0;i<SIZE;i++){
        if(mat[i][0]==mat[i][1]&&mat[i][1]==mat[i][2]&&mat[i][2]!=(char)'*'){
            return mat[i][2]=='o';
        }
    }
    //per col
    for(int i=0;i<SIZE;i++){
        if(mat[0][i]==mat[1][i]&&mat[1][i]==mat[2][i]&&mat[2][i]!=star){
            printf("winner: %d, %c\n",i, mat[2][i]);
            return mat[2][i]==o;
        }
    }
    //dig
    if(mat[0][0]==mat[1][1]&&mat[1][1]==mat[2][2]&&mat[2][2]!=(char)'*'){
        return mat[0][0]=='o';
    }
    if(mat[0][2]==mat[1][1]&&mat[1][1]==mat[2][0]&&mat[1][1]!=(char)'*'){
        return mat[1][1]=='o';
    }
    // return 2;
    //need to continue?
    // int flag=0;
    for(int i=0;i<SIZE;i++){
        for(int j=0;j<SIZE;j++){
            if(mat[i][j]=='*')return 3;
        }
    }
    return 2;

}

// static int check_()
// {
// 	int grade_sum[8];
// 	grade_sum[0]=grade[0][0]+grade[0][1]+grade[0][2];
// 	grade_sum[1]=grade[1][0]+grade[1][1]+grade[1][2];
// 	grade_sum[2]=grade[2][0]+grade[2][1]+grade[2][2];
// 	grade_sum[3]=grade[0][0]+grade[1][0]+grade[2][0];
// 	grade_sum[4]=grade[0][1]+grade[1][1]+grade[2][1];
// 	grade_sum[5]=grade[0][2]+grade[1][2]+grade[2][2];
// 	grade_sum[6]=grade[0][0]+grade[1][1]+grade[2][2];
// 	grade_sum[7]=grade[0][2]+grade[1][1]+grade[2][0];
// 	for(int i=0;i<8;i++)
// 	{
// 		if(grade_sum[i]==12) return 1;
// 		else if(grade_sum[i]==3) return 2;
// 		else return 0;
// 	}
// }

// #undef SIZE

// #include <stdio.h>

// static char USER='O';
// static char AI='X';

// #define SIZE 3

// static char mat[SIZE][SIZE]={
//     '*','*','*','*','*','*','*','*','*'
// };

// //
// static void print()
// {
//     for(int i=0;i<SIZE;i++){
//         for(int j=0;j<SIZE;j++){
//             printf("%c ", mat[i][j]);
//         }
//         printf("\n");
//     }
// }

// /*
//     @return 1: you win
//             0: you lose
//             2: equal
//             3: continue
//  */

// static int valid(int x,int y)
// {
//     return x>0&&y>0&&x<=SIZE&&y<=SIZE&&
//         mat[x-1][y-1]=='*';
// }

// static void ai_policy(int *x, int *y)
// {
//     assert(x!=0&&y!=0);
//     for(int i=0;i<SIZE;i++){
//         for(int j=0;j<SIZE;j++){
//             if(mat[i][j]=='*'){
//                 *x=i,*y=j;return;
//             }
//         }
//     }
//     assert(0);
// }

// int main_ttt()
// {
//     static char input_buf[4];
//     int x,y;
//     //
    
//     for(int times=0;times<SIZE*SIZE;times++){
        
//         if(times%2==0){
//             print();// print the matrix
//             printf("Input like: x y\n");
//             while(1){
//                 int r=read(0,input_buf,4);
//                 input_buf[r]=0;
//                 x=input_buf[0]-48;
//                 y=input_buf[2]-48;
//                 if(!valid(x,y)){
//                     printf("Input again\n");
//                     continue;
//                 }
//                 //
//                 mat[x-1][y-1]=USER;
//                 break;
//             }            
//         }else{
//             printf("????\n");
//             ai_policy(&x,&y);
//             mat[x-1][y-1]=AI;
//         }
//         // print();
//         int res=check();
//         if(res==2)continue;
//         if(res==0){
//             printf("You win!\n");
//         }else if(res==1){
//             printf("You lose!\n");
//         }else if(res==3){
//             //do nothing
//         }else if(res==2){
//             printf("No winner, no loser!\n");
//         }else{
//             printf("Error!\n");
//         }
//     }
//     return 0;
// }

// #undef SIZE