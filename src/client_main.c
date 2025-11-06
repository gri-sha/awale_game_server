#include "client/client.h"
#include "utils/constants.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
   const char *address = SERVER_ADDR;
   const char *name = NULL;

   // Parse command-line arguments
   // --name <pseudo> --ip <address>
   // if ip not provided, use default SERVER_ADDR
   for (int i = 1; i < argc; i++)
   {
      if (strcmp(argv[i], "--name") == 0)
      {
         if (i + 1 < argc)
         {
            name = argv[i + 1];
            i++;
         }
         else
         {
            printf("Error: --name requires an argument\n");
            return EXIT_FAILURE;
         }
      }
      else if (strcmp(argv[i], "--ip") == 0)
      {
         if (i + 1 < argc)
         {
            address = argv[i + 1];
            i++;
         }
         else
         {
            printf("Error: --ip requires an argument\n");
            return EXIT_FAILURE;
         }
      }
   }

   if (name == NULL)
   {
      printf("Usage: %s --name <pseudo> [--ip <address>]\n", argv[0]);
      printf("Default IP: %s\n", SERVER_ADDR);
      return EXIT_FAILURE;
   }

   app(address, name);
   return EXIT_SUCCESS;
}