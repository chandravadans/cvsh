#include<stdio.h>
#include<string.h>
#include <stdlib.h>
#include<fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <limits.h>
#include<errno.h>
#include<iostream>
#include<vector>
#include<fstream>
#include<string>
#include<iterator>
#include<signal.h>
#include<libgen.h>
#include<unistd.h>


# define FMODE (S_IRUSR | S_IWUSR)
#define FFLAG (O_WRONLY | O_CREAT | O_TRUNC)
#define FFLAGA (O_CREAT|O_WRONLY| O_APPEND)
#define MAX_CMD 512


char *present_dir;
char currdir[512];
char PROMPT[512];
using namespace std;
vector<int> bgpidlist;         //For list of bg procs


/* Function: parse
   Args    : *s         -> string to be parsed
             *delimiters-> tokenise wrt delimiters 
             **argvp    -> ptr to an array of strings, in which tokens would be placed.
   Return  : Number of tokens.
*/
int parse(const char *s, const char *delimiters, char ***argvp) 
{
  int numtokens;
  const char *newstr;
  
  if ((s == NULL) || (delimiters == NULL) || (argvp == NULL)) 
  {
    return -1;
  }

  *argvp = NULL;

  newstr = s + strspn(s, delimiters);         // strspn walks down the string till the char in delimiters is NOT found

  char *t = (char *)malloc(strlen(newstr)) + 1;

  strcpy(t, newstr);               
  numtokens = 0;
  if (strtok(t, delimiters) != NULL)         // count the number of tokens in s 
  {
    numtokens=1;
    while(strtok(NULL,delimiters)!=NULL)
      numtokens++;

  }
  *argvp=(char **)malloc((numtokens+1)*sizeof(char *));

  //Copy tokens into argvp
  strcpy(t, newstr);
  **argvp = strtok(t, delimiters);

  for (int i = 1; i < numtokens; i++)
    *((*argvp) + i) = strtok(NULL, delimiters);
  *((*argvp) + numtokens) = NULL;             
  return numtokens;
}     

/* Function : inputredirect. Handles input redirection by duping.
   Args     : cmd, the command 
   Returns  : 0, if no ipredirection (or) successful ip redirection
              -1, if error
*/
int inputredirect(char *cmd) 
{  
  // Input redirection if '<' 
   int inputfd;
   char *inputfile;

   if ((inputfile = strchr(cmd, '<')) == NULL)       //First occurrence of '<' in a string
      return 0;

   *inputfile = 0;                                   // take everything after '<' out of cmd 
   inputfile = strtok(inputfile + 1, " \t");
   
   if (inputfile == NULL)
      return 0;
   
   if ((inputfd = open(inputfile, O_RDONLY)) == -1)
   { 
     perror("line 87. opening inputfd failed.");  
     return -1;
   }
   
   if (dup2(inputfd, STDIN_FILENO) == -1)           //Put STDIN onto inputfd 
   {
      perror("line 94. Duping failed");
      close(inputfd);
      return -1;
   }
   return close(inputfd);
}

/* Function : outputredirect. Handles output redirection by duping.
   Args     : cmd, the command 
   Returns  : 0, if no ipredirection (or) successful ip redirection
              -1, if error
*/
int outputredirect(char *cmd) 
{  
  //Output redirection if '>' 
   int outputfd;
   char *outputfile;

   if ((outputfile = strchr(cmd, '>')) == NULL)
      return 0;
   
   //Take care of >>
   if(*(outputfile+1)=='>')
   {
     *outputfile=0;
     outputfile=strtok(outputfile+2,"\n");
    if (outputfile == NULL)
       return 0;
     if ((outputfd = open(outputfile, FFLAGA, FMODE)) == -1)
     {
       perror("Line 120. Opening outputfd failed");
       return -1;
     }

     if (dup2(outputfd, STDOUT_FILENO) == -1) 
     {
       perror("Line 126. duping failed");
       close(outputfd);
       return -1;
     }
     return close(outputfd);

   }
   else
   {
     *outputfile = 0;                  /* take everything after '>' out of cmd */
     outputfile = strtok(outputfile + 1, " \t");  

     if (outputfile == NULL)
       return 0;
     if ((outputfd = open(outputfile, FFLAG, FMODE)) == -1)
     {
       perror("line 142. opening outputfile failed.");
       return -1;
     }

     if (dup2(outputfd, STDOUT_FILENO) == -1) 
     {
       perror("line 148. dup failed");
       close(outputfd);
       return -1;
     }
     return close(outputfd);
   }
}

/*
  Function: handleredirect. Takes care of the whole redirection business by calling inputredirect and outputredirect
  Args    : s, command
            in, input redirection to be handled
            out, output redirection to be handled
  Returns : Nothing
*/
   
void handleredirect(char *s, int in, int out) 
{
   char **chargv;
   char *inputpos;
   char *outputpos;
   inputpos=strchr(s,'<');            //Check which one of them has occurred later, and execute that
   outputpos=strchr(s,'>');

   if ( (in==1) && (inputpos != NULL) &&  (out==1) && (outputpos != NULL) && (inputpos > outputpos) )   //Both are present, input comes later 
   {
      if (inputredirect(s) == -1) 
      { 
        //input redirection is last on line.
         perror("Failed to redirect input");
         return;
      }
      in = 0;
   }
   if ((out==1) && (outputredirect(s) == -1))
      perror("Failed to redirect output");
   else if ( (in==1) && (inputredirect(s) == -1))
      perror("Failed to redirect input");
   else if (parse(s, " \t", &chargv) <= 0)
      perror("Failed to parse command line");
   else 
   {
      if((execvp(chargv[0], chargv))<0);
      perror("Failed to execute command");
   }
   exit(1);
}

/*
   Heart of the shell.
   Function: runcmd. Handles commands with multiple redirection and/or piping.
   Args    : cmds, the command to be processed.
*/
void runcmd(char *cmds) 
{
   int pid_child;
   int numpipes;
   int fds[2];
   
   char **pipelist;                         //List of commands pipelined
   numpipes = parse(cmds, "|", &pipelist);
   int i; 
   if (numpipes <= 0) 
   {
      perror("Failed to find any commands\n");
      exit(1);
   }
   for (i = 0; i < numpipes - 1; i++) 
   {                                                            //dup everything but last
      if (pipe(fds) == -1)
         perror("Failed to create pipes");
      else if ((pid_child = fork()) == -1) 
         perror("Failed to create process to run command");
      else if (pid_child!=0) 
      {                                                         //parent
         if (dup2(fds[1], STDOUT_FILENO) == -1)
            perror("Failed to connect pipe to stdout");
         if (close(fds[0]) || close(fds[1]))
           perror("Failed to close pipes");
         handleredirect(pipelist[i], i==0, 0);
         exit(1);
      }
      if (dup2(fds[0], STDIN_FILENO) == -1)                    //child
         perror("Failed to connect last component");
      if (close(fds[0]) || close(fds[1]))
         perror("Failed to do final close");
   }
   handleredirect(pipelist[i], i==0, 1);                      // handle the last one 
   exit(1);
}

/*
  Function : cleanup_zombies. Kills off all zombies spawned by shell, when it dies.
             (Makes the world a better place for the future generations. :P)

  Args     : The signal thats to be sent. Typically SIGKILL.
*/
void cleanup_zombies(int signum)
{
  cout<<"Cleaning up zombies and exiting"<<endl;
  if(bgpidlist.size()>0)
  {
    for(int i=0;i<bgpidlist.size();i++)
      kill(bgpidlist[i],SIGKILL);
  }
  exit(signum);
 
}

int main (void) 
{
  pid_t childpid;
  char inbuf[MAX_CMD];
  int len,fgbg;
  char *back;
  fstream outfile;

  string cmd;

  //Tackle History stuff by reading histfile into memory.
  vector<string> hist;
  outfile.open("histfile", fstream::app|fstream::in);    
  while(getline(outfile,cmd))                            
    hist.push_back(cmd);
  outfile.close();


  //Tackle shell variables by reading the rcfile into memory. (rcfile has NAME=VALUE pairs)
  vector<string> vars;
  fstream rcfile;                                               
  rcfile.open(".cvshrc",fstream::out|fstream::in);
  while(getline(rcfile,cmd))
    vars.push_back(cmd);
  rcfile.close(); 

  //Keep track of cmds. A Little book keeping...
  //int cmdcnt=hist.size()-1;
  //cout<<"History size is"<<cmdcnt<<" Last cmd is "<<hist[cmdcnt-1];


  //Welcome message. Soo sweet! :)
  cout<<"Welcome to cvsh. Have lots of fun! ^_^"<<endl;


  //Handle Ctrl-C for cleaning up background procs
  signal(SIGINT, cleanup_zombies);

  while(1) 
  {
    //Print the (exciting) prompt :P     
    char *now;
    now=get_current_dir_name();
    strcpy(PROMPT,"\n cvsh:");
    strcat(PROMPT,now);
    strcat(PROMPT," ~> ");
    if (fputs(PROMPT, stdout) == EOF)
      continue;

    //If no command is entered
    if (fgets(inbuf, MAX_CMD, stdin) == NULL)
    {
      continue; 
    }
    else if(strcmp(inbuf,"\n")==0)
    {
      continue; 
    }

    //Strip \n
    len = strlen(inbuf);
    if (inbuf[len - 1] == '\n')
      inbuf[len - 1] = '\0';

    //Record History in vector, as well as the file
    hist.push_back(inbuf);    
    outfile.open("histfile", fstream::app|fstream::out);    //History File
    outfile<<inbuf<<endl;
    outfile.close();
    
    if (strcmp(inbuf, "exit") == 0)
    {
      outfile.close();
      if(bgpidlist.size()>0)
      {
        for(int i=0;i<bgpidlist.size();i++)
        {
          kill(bgpidlist[i],SIGKILL);
        }
      }
      break;
    }

    //Check if cmd is '&'
    back=strchr(inbuf,'&');
    if(back==NULL)
      fgbg=0;
    else
    {
      fgbg=1;
      *back=0;
    }
    int fg=0;

    //Chk if the cmd is 'fg'
    if(strncmp(inbuf,"fg %",4)==0)
    {
      if(bgpidlist.size()>=atoi(&inbuf[5]))
      {
        //cout<<"foregrounding pid "<<bgpidlist[atoi(&inbuf[5])];
        waitpid(bgpidlist[atoi(&inbuf[5])],NULL,0);
      }
      else
        cout<<"error: Couldn't fg the requested process.";
    }

    //Chk if cmd is !
    if(inbuf[0]=='!')
    {
      //scan the rest of the cmd into a temp string
      string tempcmd=inbuf;
      tempcmd.erase(tempcmd.begin());
      vector<string>::reverse_iterator it;
      for(it=hist.rbegin();it!=hist.rend();++it)
        if(strncmp(tempcmd.c_str(),(*it).c_str(),tempcmd.size())==0)
        {
          cout<<"Matched "<<(*it)<<endl;
          if(strcmp((*it).c_str(),"exit")==0)
            cleanup_zombies(SIGINT);
          strcpy(inbuf,(*it).c_str());
          //inbuf[strlen(((*it).c_str()))]='\0';
          goto freed;
        }
    }
freed:
    int flag=0;
    int hasdollar=0;

    //Chk if cmd is 'export'
    char test[100];
    strcpy(test,inbuf);
    char *incmd=strtok(test," ");             //incmd is the root. test is the buffer.
    //cout<<incmd<<endl;
    if(strcmp(incmd,"export")==0)
    {
      char *name=strtok(NULL,"=");
      char *value=strtok(NULL,";");
      setenv(name,value,1);
      continue;
    }
    //Handle the echo family.
    else if (strcmp(incmd,"echo")==0)           
    {
      if(strcspn(inbuf,"$")==5)                //Walk till you don't get a $
      {  
        char *echoarg=strtok(NULL,"$");
        char *out=getenv(echoarg);
        if(out!=NULL)
        {
          cout<<out<<endl;
          flag=1;
        }
        else
        {
          vector<string>::iterator it2;
          for(it2=vars.begin();it2!=vars.end();++it2)
          {
            if(strncmp(echoarg,(*it2).c_str(),strlen(echoarg))==0)
            {
              char txt[100];
              strcpy(txt,(*it2).c_str());
              strtok(txt,"=");
              cout<<strtok(NULL,";")<<endl;
              flag=1;
              break;
            }
          }
        }  
        if(flag==1)
          continue;
      }
    }

    //Chk if cmd is pwd
    else if(strcmp(incmd,"pwd")==0)
    {
      char *curdir=get_current_dir_name();
      cout<<curdir<<endl;
      continue;
    }

    //Finally, check if its cd (Phew! A long day!)
    if(strcmp(incmd,"cd")==0)
    {
      char forcd[100];
      char *arg=strtok(NULL," ");
      strcpy(forcd,inbuf);
      if(strlen(forcd)==strlen(incmd)||strlen(forcd)==strlen(incmd)+1)
      {
        present_dir=get_current_dir_name();
        cout<<"Changing to"<<getenv("HOME") <<" from "<< present_dir;
        chdir(getenv("HOME"));
        continue;
      }
      if(*arg=='~')
      {
        present_dir=get_current_dir_name();
        chdir(getenv("HOME"));
        continue;
      }
      else if(strcmp(arg,"..")==0)
      {
        //present_dir=get_current_dir_name();
        char *path=get_current_dir_name();
        char *dest=dirname(path);
        if(chdir(dest)==0);
        else
          cout<<"oops!";
        continue;
      }
      else if(*arg=='-')
      {
        chdir(getenv("$OLDPWD"));
        continue;
      }
      else
      {
        present_dir=get_current_dir_name();
        if(chdir(arg)!=0)
          cout<<"Cant change to "<<arg<<" ! ^_^'"<<endl; 
        continue;
      }
    }

    //Fork a child to execute cmd
    if ((childpid = fork()) == -1)
      perror("Failed to fork child");

    else if (childpid == 0) 
    {
      if(fgbg==1 && setpgid(0,0)==-1)     //If background process, put the child in its own group
        return 1;

      runcmd(inbuf);
      return 1; 
    } 
    if(fgbg==0)
      waitpid(childpid,NULL,0);             //Wait for foreground process
    else
    {
      cout<<childpid<<" In background\n";
      bgpidlist.push_back(childpid);
      cout<<"List of bg procs"<<endl;
      copy(bgpidlist.begin(),bgpidlist.end(),ostream_iterator<int>(cout,"\n"));
      while(waitpid(-1,NULL,WNOHANG)>0);         //Wait for all background processes
    }
  }
  return 0; 
}
