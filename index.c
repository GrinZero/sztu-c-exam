#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#define COLUMN 11
#define SPLIT_STRING "交易时间,交易类型,交易对方,商品,收/支,金额(元),支付方式,当前状态,交易单号,商户单号,备注\r\n"
#define SPLIT_LINE "\n"

typedef enum
{
    time = 0,
    type = 1,
    object = 2,
    product = 3,
    income = 4,
    money = 5,
    pay = 6,
    status = 7,
    order = 8,
    merchant = 9,
    remark = 10
} Column;

struct obj
{
    char *key;
    double value;
    int count;
};

void printSplitLine(char c, int n)
{
    char *str = (char *)malloc(n + 1);
    memset(str, c, n);
    str[n] = '\0';
    printf("%s\n", str);
}

char **split(const char *source, char flag)
{
    char **pt;
    int j, n = 0;
    int count = 1;
    int len = strlen(source);
    // 动态分配一个 *tmp，静态的话，变量len无法用于下标
    char *tmp = (char *)malloc((len + 1) * sizeof(char));
    tmp[0] = '\0';

    for (int i = 0; i < len; ++i)
    {
        if (source[i] == flag && source[i + 1] == '\0')
            continue;
        else if (source[i] == flag && source[i + 1] != flag)
            count++;
    }
    // 多分配一个char*，是为了设置结束标志
    pt = (char **)malloc((count + 1) * sizeof(char *));

    count = 0;
    for (int i = 0; i < len; ++i)
    {
        if (i == len - 1 && source[i] != flag)
        {
            tmp[n++] = source[i];
            tmp[n] = '\0'; // 字符串末尾添加空字符
            j = strlen(tmp) + 1;
            pt[count] = (char *)malloc(j * sizeof(char));
            strcpy(pt[count++], tmp);
        }
        else if (source[i] == flag)
        {
            j = strlen(tmp);
            if (j != 0)
            {
                tmp[n] = '\0'; // 字符串末尾添加空字符
                pt[count] = (char *)malloc((j + 1) * sizeof(char));
                strcpy(pt[count++], tmp);
                // 重置tmp
                n = 0;
                tmp[0] = '\0';
            }
        }
        else
            tmp[n++] = source[i];
    }

    // 释放tmp
    free(tmp);

    pt[count] = NULL;

    return pt;
}

double getMoney(char *source)
{
    char **pt;
    pt = split(source, '\xc2');
    pt = split(pt[0], '\xa5');
    double moneyVal = atof(*pt);
    return moneyVal;
}

int main(int argc, char **argv)
{
    char *filename = argv[1];
    printSplitLine('-', 50);
    printf("Reading file \"%s\"\n", filename);
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL)
    {
        printf("Error opening file: fopen() failed\n");
        return 1;
    };

    fseek(fp, 0, SEEK_END);

    // the size of the file
    long flen = ftell(fp);
    printf("File length: %ld\n", flen);

    fseek(fp, 0, SEEK_SET);
    int fd = fileno(fp);
    if (fd == -1)
    {
        printf("Error opening file: fileno() failed\n");
        return 1;
    };

    char *file = mmap(NULL, flen, PROT_READ, MAP_PRIVATE, fd, 0);
    if (file == MAP_FAILED)
    {
        printf("Error opening file: mmap() failed\n");
        return 1;
    };
    fclose(fp);

    int file_line = 0;
    char *line = file;
    line = strstr(file, SPLIT_STRING);
    while (*line)
    {
        if (*line == '\r')
        {
            line = strstr(line, SPLIT_LINE);
            if (line == NULL)
            {
                break;
            }
            file_line++;
        }
        else
            line++;
    }
    printf("File line: %d\n", file_line);
    line = NULL;
    printSplitLine('-', 50);

    char buffer[file_line][COLUMN][50];
    char *p;
    p = strstr(file, SPLIT_STRING);
    p++;
    for (int i = 0; i < file_line; i++)
    {
        for (int j = 0; j < COLUMN; j++)
        {
            int index = 0;
            while (*p != ',' && *p != '\r' && *p)
            {
                buffer[i][j][index++] = *p;
                p++;
            }
            p++;
            buffer[i][j][index++] = '\0';
        }
        if (*p == '\n')
        {
            p = strstr(p, SPLIT_LINE);
            p++;
        }
    }

    munmap(file, flen);

    // 开始统计I：计算最大/小收入和最大/小支出
    double incomeOut = 0, incomeIn = 0, incomeRemain = 0;
    struct obj maxInComeOut = {NULL, 0}, maxInComeIn = {NULL, 0};
    struct obj minInComeOut = {NULL, MAXFLOAT}, minInComeIn = {NULL, MAXFLOAT};

    for (int i = 1; i < file_line; i++)
    {
        char *objectKey = buffer[i][object];
        char *moneyValue = buffer[i][money];
        double moneyVal = getMoney(moneyValue);

        char *incomeKey = buffer[i][income];
        if (strcmp(incomeKey, "支出") == 0)
        {
            incomeOut += moneyVal;
            if (moneyVal > maxInComeOut.value)
            {
                maxInComeOut.value = moneyVal;
                maxInComeOut.key = objectKey;
            }
            if (moneyVal < minInComeOut.value)
            {
                minInComeOut.value = moneyVal;
                minInComeOut.key = objectKey;
            }
        }
        else if (strcmp(incomeKey, "收入") == 0)
        {
            incomeIn += moneyVal;
            if (moneyVal > maxInComeIn.value)
            {
                maxInComeIn.value = moneyVal;
                maxInComeIn.key = objectKey;
            }
            if (moneyVal < minInComeIn.value)
            {
                minInComeIn.value = moneyVal;
                minInComeIn.key = objectKey;
            }
        }
        else
        {
            incomeRemain += moneyVal;
        }
    }

    printf("在%s~%s期间\n", buffer[file_line - 1][time], buffer[1][time]);
    printf("你的收入金额为: %.2f\n", incomeIn);
    printf("你的支出金额为: %.2f\n", incomeOut);
    printf("中性交易金额为: %.2f\n", incomeRemain);
    printf("你的收入最大值为: %.2f, 收入来源是【%s】\n", maxInComeIn.value, maxInComeIn.key);
    if (maxInComeIn.value != minInComeIn.value && maxInComeIn.key != minInComeIn.key)
        printf("你的收入最小值为: %.2f, 收入来源是【%s】\n", minInComeIn.value, minInComeIn.key);
    printf("你的支出最大值为: %.2f, 支出去向是【%s】\n", maxInComeOut.value, maxInComeOut.key);
    if (maxInComeOut.value != minInComeOut.value && maxInComeOut.key != minInComeOut.key)
        printf("你的支出最小值为: %.2f, 支出去向是【%s】\n", minInComeOut.value, minInComeOut.key);

    double totalIncome = incomeIn - incomeOut;
    printf("你的总收入为: %.2f，当前收入支出%s\n", totalIncome, totalIncome ? "正常，继续保持!" : "不合理，要少花钱哦！");

    // 开始统计II：计算各个类别的收入/支出
    struct obj categoryOut[100] = {NULL, 0, 0};
    struct obj categoryIn[100] = {NULL, 0, 0};
    int categoryOutIndex = 0, categoryInIndex = 0;

    for (int i = 1; i < file_line; i++)
    {
        char *objectKey = buffer[i][object];
        char *moneyValue = buffer[i][money];
        double moneyVal = getMoney(moneyValue);

        char *incomeKey = buffer[i][income];
        if (strcmp(incomeKey, "支出") == 0)
        {
            int index = 0;
            while (categoryOut[index].key != NULL)
            {
                if (strcmp(categoryOut[index].key, objectKey) == 0)
                {
                    categoryOut[index].value += moneyVal;
                    categoryOut[index].count++;
                    break;
                }
                index++;
            }
            if (categoryOut[index].key == NULL)
            {
                categoryOut[index].key = objectKey;
                categoryOut[index].value = moneyVal;
                categoryOut[index].count = 1;
                categoryOutIndex++;
            }
        }
        else if (strcmp(incomeKey, "收入") == 0)
        {
            int index = 0;
            while (categoryIn[index].key != NULL)
            {
                if (strcmp(categoryIn[index].key, objectKey) == 0)
                {
                    categoryIn[index].value += moneyVal;
                    categoryIn[index].count++;
                    break;
                }
                index++;
            }
            if (categoryIn[index].key == NULL)
            {
                categoryIn[index].key = objectKey;
                categoryIn[index].value = moneyVal;
                categoryIn[index].count = 1;
                categoryInIndex++;
            }
        }
    }

    printSplitLine('-', 20);
    printf("支出类数据:\n");
    for (int i = 0; i < categoryOutIndex; i++)
    {
        printf("#【%s】: %.2f\n", categoryOut[i].key, categoryOut[i].value);
    }
    printSplitLine('-', 20);
    printf("收入类数据:\n");
    for (int i = 0; i < categoryInIndex; i++)
    {
        printf("#【%s】: %.2f\n", categoryIn[i].key, categoryIn[i].value);
    }
    printSplitLine('-', 20);

    // 开始统计III：计算最大收入的类别与最大支出的类别
    struct obj maxCategoryOut = {NULL, 0};
    struct obj maxCategoryIn = {NULL, 0};
    for (int i = 0; i < categoryOutIndex; i++)
    {
        if (categoryOut[i].value > maxCategoryOut.value)
        {
            maxCategoryOut.value = categoryOut[i].value;
            maxCategoryOut.key = categoryOut[i].key;
        }
    }
    for (int i = 0; i < categoryInIndex; i++)
    {
        if (categoryIn[i].value > maxCategoryIn.value)
        {
            maxCategoryIn.value = categoryIn[i].value;
            maxCategoryIn.key = categoryIn[i].key;
        }
    }
    printf("你本周期最大的支出类别是【%s】，累计金额为: %.2f\n", maxCategoryOut.key, maxCategoryOut.value);
    printf("你本周期最大的收入源是【%s】，收入金额为: %.2f\n", maxCategoryIn.key, maxCategoryIn.value);

    // 开始统计IV：计算最多次数的收入/支出
    struct obj maxCountOut = {NULL, 0, 0};
    struct obj maxCountIn = {NULL, 0, 0};

    for (int i = 0; i < categoryOutIndex; i++)
    {
        if (categoryOut[i].count > maxCountOut.count)
        {
            maxCountOut.count = categoryOut[i].count;
            maxCountOut.key = categoryOut[i].key;
            maxCountOut.value = categoryOut[i].value;
        }
    }
    for (int i = 0; i < categoryInIndex; i++)
    {
        if (categoryIn[i].count > maxCountIn.count)
        {
            maxCountIn.count = categoryIn[i].count;
            maxCountIn.key = categoryIn[i].key;
            maxCountIn.value = categoryIn[i].value;
        }
    }
    printf("你本周期最多次数的支出类别是【%s】，累计次数：%d次，累计金额：%.2f\n", maxCountOut.key, maxCountOut.count, maxCountOut.value);
    printf("你本周期最多次数的收入源是【%s】，收入次数：%d次，收入金额：%.2f\n", maxCountIn.key, maxCountIn.count, maxCountIn.value);
    // for (int i = 0; i < file_line; i++)
    // {
    //     for (int j = 0; j < COLUMN; j++)
    //     {
    //         printf("%s|", buffer[i][j]);
    //     }
    //     printf("\n####");
    // }
    printSplitLine('-', 50);
    printSplitLine('#', 30);
    printf("输入交易单号或者商户单号查询具体交易信息（Ctrl+C退出）\n");
    printSplitLine('#', 30);

    char inputNum[100];
    char *inputNumPtr;
    while (fgets(inputNum, 100, stdin) != NULL)
    {
        inputNumPtr = split(inputNum, '\n')[0];
        for (int i = 1; i < file_line; i++)
        {
            char *orderNum = split(buffer[i][order], '\t')[0];
            char *merchantNum = split(buffer[i][merchant], '\t')[0];
            if (strcmp(inputNumPtr, orderNum) == 0 || strcmp(inputNumPtr, merchantNum) == 0)
            {
                printf("交易单号: %s\n", orderNum);
                printf("商户单号: %s\n", merchantNum);
                printf("交易时间: %s\n", buffer[i][time]);
                printf("交易金额: %s\n", buffer[i][money]);
                printf("交易类型: %s\n", buffer[i][income]);
                printf("交易状态: %s\n", buffer[i][status]);
                printf("交易备注: %s\n", buffer[i][remark]);
                break;
            }
        }
        printSplitLine('#', 30);
        printf("输入交易单号或者商户单号查询具体交易信息（Ctrl+C退出）\n");
        printSplitLine('#', 30);
        printf("\n");
    }
    printSplitLine('-', 50);

    return 0;
}