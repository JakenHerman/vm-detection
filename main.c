//
//  main.c
//  guacamole
//
//  Created by Jaken Herman on 7/13/16.
//  Copyright © 2016 jaken herman. All rights reserved.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//check to see if operating system is windows
#ifdef _WIN32
#include <windows.h> //include <windows.h> for win operating systems only
//if not windows, assume unix based system
#else
#include <unistd.h> //include <unistd.h> for unix systems only.
#endif

/* Use variable vm_score in order to keep track of likelihood of virtual 
machine. After each check, we will increment vm_score by 1 if the check
passes. If the vm_score reaches 3, we assume that we are running on a 
virtual machine.
*/
int vm_score = 0;

void number_of_cores();
void run_command();
void registry_check();

int main(int argc, const char * argv[]) {
    /*run number_of_cores() function first, 
    as it runs on both windows and unix machines.
    */
    number_of_cores();
    
//check to see if running on windows.
#ifdef WIN32
    /*vmware_sys holds the value that the systeminfo command should return for
    the System Manufacturer value. */
    char* vmware_sys = "System Manufacturer: \t VMware, Inc.";
    
    /*Use the run_command function we created in order to run systeminfo piped
    to find the System Manufacturer, and compare it with the vmware_sys variable,
    which is what should be returned if a hypervisor is running. The 36 as the
    last parameter allows us to dictate that the string should be 36 characters long.
    */
    run_command("systeminfo | find \"System Manufacturer\"", vmware_sys, 36);
    
    //run the registry_check() function to check for vmware in the registers.
    registry_check();
    
    /*If vm_score is less than 3, we are likely running on physical hardware*/
    if(vm_score < 3){
      printf("no virtual machine detected");
    }
    printf("Virtual Machine detected.");
	   
//The code below only runs if we are not on a Windows-based system. Only tested with openSUSE.
#else
    
    //run the dmesg command and pipe to find hypervisor, 34 is how long string should be.
    run_command("dmesg |grep -i hypervisor", "[   0.000000 Hypervisor detected]", 34);
    
    //run dmidecode command and find system manufacturer, 6 is how long the string should be.
    run_command("sudo dmidecode -s system-manufacturer", "VMware", 6);
    
    /*If vm_score is less than 3, we are likely running on physical hardware*/
    if(vm_score < 3){
      printf("No virtual machine detected");
    }

    printf("Virtual Machine detected.");
#endif
    return 0;
}

/* number_of_cores serves the purpose of checking to see how many
   cores the system is running on. If the core count is less than
   one, there is a very good chance we are running on a virtual
   machine. */

void number_of_cores() {
//check to see if running on windows.
#ifdef WIN32
    //use SYSTEM_INFO structure from WinAPI to get system info
    SYSTEM_INFO sysinf; 
    //Run the GetSystemInfo functin pointing to our SYSTEM_INFO structure.
    GetSystemInfo(&sysinf);
    //check if number of processors is less than or equal to one. If it is,
    //we assume virtual, and increment vm_score by 1.
    if(sysinf.dwNumberOfProcessors<=1){
        vm_score++;
    }
//Not windows? Run Unix code
#else
    //run sysconf function outlined in the man pages.
    if(sysconf(_SC_NPROCESSORS_ONLN) <= 1){
        //check if number of processors is less than or equal to one. If it is,
        //we assume virtual, and increment vm_score by 1.
        vm_score++;
    }
#endif
} //end of number_of_cores()


/* run_command serves the purposes of running terminal commands
   within both linux and windows environments.
   We use this for dmesg, dmidecode, and systeminfo.
   
   run_command() takes three parameters: cmd, detphrase, and dp_length.
   cmd is the command to run such as systeminfo or dmesg or dmidecode
   
   detphrase is the string to check for within the command. If we run
   dmidecode and we're looking for "VMWARE", the detphrase is "VMWARE".
   
   dp_length is the length of the string we want to specify. This allows
   us to take substrings of the system output in order to have cleaner, 
   more readable code. 
   
   The benefits of using this run_command() function over a system() call
   is that it allows us to save the output of system calls as well as take
   substrings.
*/
void run_command(char *cmd, char *detphrase, int dp_length){
    #define BUFSIZE 128
    char buf[BUFSIZE];
    FILE *fp;

    /*popen() is essentially the same as system()
    but it saves the output to a file. If the output
    is null, the command didn't work.
    */
    if((fp = popen(cmd, "r")) == NULL){
        printf("Error");
    }

    
    if(fgets(buf, BUFSIZE, fp) != NULL){
        char detection[(dp_length +1 )]; //one extra char for null terminator
        strncpy(detection, detphrase, dp_length);
        detection[dp_length] = '\0'; //place the null terminator

        if(strcmp(detphrase, detection) == 0){ //0 means detphrase = detection
            vm_score++; //increment the vm_score variable.
        }
    }

    if(pclose(fp)){
        printf("Command not found or exited with error status \n");
    }
}

/* registry_check is a modified version of Sudeep Singh's
   registry_check method in the Breaking the Sandbox document.
   
   It is modified to only check for vmware, as that is the hypervisor
   the rest of the methods in this program check for.
*/
#ifdef WIN32
void registry_check(){
    HKEY hkey;
    char *buffer;
    int i=0,j=0;
    int size = 256;
    char *vm_name = "vmware";
    buffer = (char *) malloc(sizeof(char) * size);

    /* Use RegOpenKeyEx and RegQueryValueEx, as described in the 
    WINAPI in order to look at memory registers and look for the
    vm_name, in this case, "vmware".*/
    RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                 "SYSTEM\\ControlSet001\\Services\\Disk\\Enum",
                 0, KEY_READ, &hkey);
    RegQueryValueEx(hkey, "0", NULL, NULL, buffer, &size);

    while(*(buffer+i)){
        *(buffer+i) = (char) tolower(*(buffer+i));
        i++;
    }

    //compare the buffer and "vmware" to see if they are the same.
    if(strstr(buffer, vm_name) != NULL){
        vm_score++; //if buffer and "vmware" are equal, increase vm_score
    }

    return;
}
#endif
