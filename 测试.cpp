#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

int main()
{
    int i, j, x, a[4][5], tag = 0;
    
    srand((unsigned)time(0));
    
    
    for(i = 0; i < 4; i++)
    {
        for(j = 0; j < 5; j++)
        {
            a[i][j] = rand() % 100;
            printf("%2d ", a[i][j]); 
        }
        printf("\n");
    }
    
    printf("Input a data: ");
    scanf("%d", &x); // 修复 scanf 
    
    // 查找
    for(i = 0; i < 4; i++)
    {
        for(j = 0; j < 5; j++)
        {
            if(x == a[i][j])
            {
                tag = 1;
                break; // 找到了就跳出
            }
        }
        if(tag == 1) break; // 如果内层找到了，外层也直接跳出
    }
    
    // 输出
    if(tag) 
    {
        printf("OK\n");
    }
    else
    {
        printf("NO\n");
    }
    
    system("pause"); 
    return 0;
}


