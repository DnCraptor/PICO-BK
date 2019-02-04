#include <stdio.h>
#include <string.h>

typedef int BOOL;
#define FALSE 0
#define TRUE 1

long int Label [65536][32];
long int TempLabel [32];
long int CrtLabel [32];
long int CountLabel = 0;
long int NString = 0;
char InclFiles[1000];

int ExitCode = 0;
int CommentStile = 0;
int LabelStile = 0;

char *SkipNumber (char *pSt)
{
    while ((*pSt == '-') || ((*pSt >= '0') && (*pSt <='9')))
    {
        pSt = pSt + 1;
    }
    return pSt;
}


void CreateLabel (FILE *InputFile, int CountInv)
{
    long int Step = 0;
    int Out, Stp;
    char St[1000];
    char St1[1000];
    char *pSt;
    int i,i1;
    int LenName;

    long int TempNString;
    FILE *IncludeFile;
    long int FilePos = ftell (InputFile);
    
    while (1)
    {
        if (fgets (St, 1000, InputFile) == NULL)
        {
            if (FilePos == 0) break;
            else
            {
                printf ("    End of file Error;\n");
                ExitCode = 1;
                break;
            }
        }
        NString++;
        TempLabel [CountInv] = ++Step;
        St1[0] = 0;
        sscanf (St, "%s", &St1);
        if (St1[0] == '{') CreateLabel (InputFile, CountInv+1);
        else if (St1[0] == '}')
        {
            if (CountInv > 0) break;
            else
            {
                printf ("    Syntax Error; String=%ld\n", NString);
                ExitCode = 1;
                break;
            }
        }
/*
        else if (St1[0] == '#')
        {
            IncludeFile = fopen (St1+1,"rt");
            if (IncludeFile == NULL) printf ("\n    Included file not found; String=%ld\n", NString);
            else
            {
                TempNString = NString;
                NString = 0;
                LenName = strlen (InclFiles);
                strcat (InclFiles, " > ");
                strcat (InclFiles, St1+1);
                if (strlen(InclFiles) > 75) printf ("..%s", InclFiles + strlen(InclFiles) - 73);
                else printf ("%s", InclFiles);
                for (i = strlen (InclFiles); i < 75; i++) printf (" ");
                printf ("\r");
                CreateLabel (IncludeFile, CountInv+1);
                InclFiles[LenName] = 0;
                if (strlen(InclFiles) > 75) printf ("..%s", InclFiles + strlen(InclFiles) - 73);
                else printf ("%s", InclFiles);
                for (i = strlen (InclFiles); i < 75; i++) printf (" ");
                printf ("\r");
                NString = TempNString;
                fclose (IncludeFile);
            }
        }
*/
        else
        {
            pSt = St;
            while ((pSt = strstr (pSt, "!")) != NULL)
            {
                pSt = pSt + 1;
                if ((*pSt != 'o') && (*pSt != 'n') && (*pSt != 'p')) continue;
                Out = 0;
                Stp = 0;
                if (*pSt == 'o')
                {
                    sscanf (pSt + 1, "%d", &Out);
                    pSt = SkipNumber (pSt+1);
                }
                if (*pSt == 'n')
                {
                    sscanf (pSt + 1, "%d", &Stp);
                    pSt = SkipNumber (pSt+1);
                }
                else if (*pSt == 'p')
                {
                    sscanf (pSt + 1, "%d", &Stp);
                    Stp = 0 - Stp;
                    pSt = SkipNumber (pSt+1);
                }
                if (((CountInv - Out) >= 0) && ((TempLabel[CountInv - Out] + 1 + Stp) > 0))
                {
                    for (i = 0; i <= (CountInv - Out); i++) CrtLabel[i] = TempLabel[i];
                    CrtLabel [CountInv - Out] += 1 + Stp;
                    CrtLabel [CountInv - Out + 1] = 0;
                    for (i = 0; i < CountLabel; i++)
                    {
                        for (i1 = 0; i1 <= (CountInv - Out + 1); i1++)
                        {
                            if (Label[i][i1] != CrtLabel[i1]) break;
                        }
                        if (i1 > (CountInv - Out + 1)) break;
                    }
                    if (i >= CountLabel)
                    {
                        for (i = 0; i <= (CountInv - Out + 1); i++)
                            Label[CountLabel][i] = CrtLabel[i];
                        CountLabel++;
                    }
                }
                else
                {
                    printf ("    Invalid label; String=%ld\n", NString);
                    ExitCode = 1;
                    break;
                }
            }
        }
    }
}

void Convert (FILE *InputFile, FILE *OutputFile, int CountInv)
{
    long int Step = 0;
    int Out, Stp;
//    BOOL fRem;
    char St[1000];
    char St1[1000];
    char St2[1000];
    char *pSt;
    int i,i1;
    int LenName;

    long int TempNString;
    FILE *IncludeFile;
    long int FilePos = ftell (InputFile);

    while (1)
    {
        if (fgets (St, 1000, InputFile) == NULL)
        {
            if (FilePos == 0) break;
            else
            {
                printf ("    End of file Error;\n");
                ExitCode = 1;
                break;
            }
        }
//        fRem = FALSE;
        NString++;
        TempLabel [CountInv] = ++Step;
        TempLabel [CountInv+1] = 0;
        
        for (i = 0; i < CountLabel; i++)
        {
            for (i1 = 0; i1 <= (CountInv + 1); i1++)
            {
                if (Label[i][i1] != TempLabel[i1]) break;
            }
            if (i1 > (CountInv + 1)) break;
        }
        if (i < CountLabel)
        {
            if (LabelStile == 1) fprintf (OutputFile, "_M%dAUTO:\n", i);
            else                 fprintf (OutputFile, "_M%dAUTO\n", i);
        }
        
        St1[0] = 0;
        sscanf (St, "%s", &St1);
        if (St1[0] == '{')
        {
//            if (fRem == FALSE)
//            {
            if (CommentStile == 1) fprintf (OutputFile, ";%s", St);
            else
            {
                int Count;
                char Ch;

                for (Count = 0; Ch = St [Count], Ch != '{' && Ch != 0; Count++) fprintf (OutputFile, "%c", Ch);
                if (Ch != 0) fprintf (OutputFile, "/* { */%s", St + (Count + 1));
            }

//                fRem = TRUE;
//            }
            Convert (InputFile, OutputFile, CountInv+1);
        }
        else if (St1[0] == '}')
        {
//            if (fRem == FALSE)
//            {
            if (CommentStile == 1) fprintf (OutputFile, ";%s", St);
            else
            {
                int Count;
                char Ch;

                for (Count = 0; Ch = St [Count], Ch != '}' && Ch != 0; Count++) fprintf (OutputFile, "%c", Ch);
                if (Ch != 0) fprintf (OutputFile, "/* } */%s", St + (Count + 1));
            }
//                fRem = TRUE;
//            }
            if (CountInv > 0) break;
            else
            {
                printf ("    Syntax Error; String=%ld\n", NString);
                ExitCode = 1;
                break;
            }
        }
/*
        else if (St1[0] == '#')
        {
            IncludeFile = fopen (St1+1,"rt");
            if (IncludeFile == NULL) printf ("\n    Included file not found; String=%ld\n", NString);
            else
            {
                TempNString = NString;
                NString = 0;
                LenName = strlen (InclFiles);
                strcat (InclFiles, " > ");
                strcat (InclFiles, St1+1);
                if (strlen(InclFiles) > 75) printf ("..%s", InclFiles + strlen(InclFiles) - 73);
                else printf ("%s", InclFiles);
                for (i = strlen (InclFiles); i < 75; i++) printf (" ");
                printf ("\r");
                Convert (IncludeFile, OutputFile, CountInv+1);
                InclFiles[LenName] = 0;
                if (strlen(InclFiles) > 75) printf ("..%s", InclFiles + strlen(InclFiles) - 73);
                else printf ("%s", InclFiles);
                for (i = strlen (InclFiles); i < 75; i++) printf (" ");
                printf ("\r");
                NString = TempNString;
                fclose (IncludeFile);
            }

        }
*/
        else
        {
            pSt = St;

            while (*pSt != 0)
            {
                
                if ((*pSt == '!') && ((pSt[1] == 'o') || (pSt[1] == 'n') || (pSt[1] == 'p')))
                {
                    pSt = pSt + 1;
                    Out = 0;
                    Stp = 0;
//                    fRem = TRUE;
                    if (*pSt == 'o')
                    {
                        sscanf (pSt + 1, "%d", &Out);
                        pSt = SkipNumber (pSt+1);
                    }
                    if ((*pSt == 's') || (*pSt == 'n'))
                    {
                        sscanf (pSt + 1, "%d", &Stp);
                        pSt = SkipNumber (pSt+1);
                    }
                    else if (*pSt == 'p')
                    {
                        sscanf (pSt + 1, "%d", &Stp);
                        Stp = 0 - Stp;
                        pSt = SkipNumber (pSt+1);
                    }
                    if (((CountInv - Out) >= 0) && ((TempLabel[CountInv - Out] + 1 + Stp) > 0))
                    {
                        for (i = 0; i <= (CountInv - Out); i++) CrtLabel[i] = TempLabel[i];
                        CrtLabel [CountInv - Out] += 1 + Stp;
                        CrtLabel [CountInv - Out + 1] = 0;
                        for (i = 0; i < CountLabel; i++)
                        {
                            for (i1 = 0; i1 <= (CountInv - Out + 1); i1++)
                            {
                                if (Label[i][i1] != CrtLabel[i1]) break;
                            }
                            if (i1 > (CountInv - Out + 1)) break;
                        }
                        if (i < CountLabel)
                        {
                            fprintf (OutputFile, "_M%dAUTO", i);
                        }
                        else
                        {
                            printf ("    Label not found; String=%ld\n", NString);
                            ExitCode = 1;
                            break;
                        }
                    }
                    else
                    {
                        printf ("    Invalid label; String=%ld\n", NString);
                        ExitCode = 1;
                        break;
                    }
                }
				else if ((*pSt == '!') && (pSt[1] == ':'))
				{
                    if (LabelStile == 1) fprintf (OutputFile, ":");
                    pSt = pSt + 2;
				}
                else
                {
                    fprintf (OutputFile, "%c", *pSt);
                    pSt = pSt + 1;
                }
            }
        }
    }
}

int main( int argc, char *argv[] )
{
    FILE *InputFile;
    FILE *OutputFile;
    int i, i1;
    for (i=0; i<65536; i++) for (i1=0; i1<32; i1++) Label [i][i1] = 0;

    if (argc != 5)
    {
        printf ("   Invalid Parameters\n\n");
        printf ("   Use: ask2asm.exe CommentStile LabelStile input_file output_file\n\n");
        printf ("   CommentStile can be:\n\n");
        printf ("       1 - asm comment stile ;\n");
        printf ("       2 - c comment stile /* */\n\n");
        printf ("   LabelStile can be:\n\n");
        printf ("       1 - label name with colon\n");
        printf ("       2 - label name without colon\n");
        ExitCode = 1;
        goto Exit;
    }

    if (sscanf (argv[1], "%d", &CommentStile) != 1 || CommentStile < 1 || CommentStile > 2)
    {
        printf ("    Invalid comment stile\n", argv[1]);
        ExitCode = 1;
        goto Exit;
    }

    if (sscanf (argv[2], "%d", &LabelStile) != 1 || LabelStile < 1 || LabelStile > 2)
    {
        printf ("    Invalid Label stile\n", argv[2]);
        ExitCode = 1;
        goto Exit;
    }

    InputFile = fopen (argv[3], "rt");
    if (InputFile == NULL)
    {
        printf ("    File %s not found\n", argv[3]);
        ExitCode = 1;
        goto Exit;
    }

    OutputFile = fopen (argv[4], "wt");
    if (OutputFile == NULL)
    {
        printf ("    File %s not created\n", argv[4]);
        ExitCode = 1;
        goto Exit1;
    }

    NString = 0;
/*
    strcpy (InclFiles, argv[2]);
    if (strlen(InclFiles) > 75) printf ("..%s", InclFiles + strlen(InclFiles) - 73);
    else printf ("%s", InclFiles);
    for (i = strlen (InclFiles); i < 75; i++) printf (" ");
    printf ("\r");
*/
    CreateLabel (InputFile, 0);

    if (ExitCode) goto Exit2;

    fseek (InputFile, 0, SEEK_SET);
    NString = 0;
/*
    if (strlen(InclFiles) > 75) printf ("..%s", InclFiles + strlen(InclFiles) - 73);
    else printf ("%s", InclFiles);
    for (i = strlen (InclFiles); i < 75; i++) printf (" ");
    printf ("\r");
*/
    Convert (InputFile, OutputFile, 0);

Exit2:

    fclose (OutputFile);

Exit1:

    fclose (InputFile);

Exit:
    if (ExitCode == 0) printf ("End Convert\n");
    return ExitCode;
}
