#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

/* Method for processing actions triggered by keybinds */
void process_action(char *action)
{	
	/* Process shell commands */
	char *instruction_signifier = "wcmd ";
	if(memcmp(action, instruction_signifier, strlen(instruction_signifier)) != 0)
	{
		pid_t pid = fork();
		
		/* Handle shell error */
		if (pid == 0)
		{		
			char *const args[] = {"/bin/sh", "-c", action, NULL};
			execv("/bin/sh", args);
			printf("[Wormhole] Failed to execute shell command.\n");
			exit(1);	
		}
		
		/* Handle process-forking error */
		if (pid < 0)
		{ 
			printf("[Wormhole] Failed to fork process.\n");
			return;
		}
	}
	
	/* Process Wormhole-specific actions */
	else
	{
		// TODO - implement
	}
}