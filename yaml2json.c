/* todo:
 *  -  make commandline flags work
 *  -  make it so you can do different things with multiline
 *  -  write tests
 *  - make so multiline only for values, make an error in keys
 *  - Document crazy magic letter and make some why comments
 * */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <yaml.h>

typedef char state;

typedef struct {
  state *contents;
  int maxSize;
  int top;
} Stack;

int StackIsFull(Stack *stackP) { return stackP->top >= stackP->maxSize - 1; }

int StackIsEmpty(Stack *stackP) { return stackP->top < 0; }

void StackContents(Stack *stackP) { 
    printf("\nSTACK: (%d): ", stackP->top);
    for (int c = 0; c <= stackP->top ; c++){
        printf("%c", *((stackP->contents)+c));
    }
    printf("\n");
}

void StackInit(Stack *stackP, int maxSize) {
  state *newContents;

  newContents = (state *)malloc(sizeof(state) * maxSize);

  if (newContents == NULL) {
    fprintf(stderr, "Insufficient memory to initialize stack.\n");
    exit(1);
  }

  stackP->contents = newContents;
  stackP->maxSize = maxSize;
  stackP->top = -1; /* I.e., empty */
}

void StackDestroy(Stack *stackP) {
  free(stackP->contents);

  stackP->contents = NULL;
  stackP->maxSize = 0;
  stackP->top = -1;  /* I.e., empty */
}

void Push(Stack *stackP, state element) {
  if (StackIsFull(stackP)) {
    fprintf(stderr, "Can't push element on stack: stack is full.\n");
    exit(1);
  }
  stackP->contents[++stackP->top] = element;
}

state Pop(Stack *stackP) {
  if (StackIsEmpty(stackP)) {
    fprintf(stderr, "Can't pop element from stack: stack is empty.\n");
    exit(1);
  }
  return stackP->contents[stackP->top--];
}

state Peak(Stack *stackP) {
  if (StackIsEmpty(stackP)) {
      return 'Q';
  }
  return stackP->contents[stackP->top];
}

void multiLineToArray(unsigned char * unsigned_string, int pretty){
    const char newline = '\n';
    char *line;
    char *string = unsigned_string; //not sure how to get around this in the best way

    if (strchr(string, newline) == NULL){
        printf("\"%s\"",string);
    } else {
	printf("[");
        line = strtok(string,&newline);
        printf(" \"%s\"", line);
        while (line != NULL){
            line = strtok(NULL, &newline);
	    if (line != NULL){
                printf(", \"%s\"", line);
            }
	}
	printf("]");
    }
}

int main(void) {
  FILE *fh = fopen("test.yaml", "r");  //TODO - make this an input variable
  int maxStackSize = 100000; //TODO - make this an input variable
  const char newline = '\n';
  char *string;

  yaml_parser_t parser;
  yaml_event_t  event;

  /* Initialize stack state so we can keep track of context in yaml */
  Stack stateStack; 
  StackInit(&stateStack, maxStackSize);

  /* Initialize parser */
  if(!yaml_parser_initialize(&parser))
    fputs("Failed to initialize parser!\n", stderr);
  if(fh == NULL)
    fputs("Failed to open file!\n", stderr);

  /* Set input file */
  yaml_parser_set_input_file(&parser, fh);

  /* START parse loop */
  do {
    if (!yaml_parser_parse(&parser, &event)) {
       printf("Parser error %d\n", parser.error);
       exit(EXIT_FAILURE);
    }
    //StackContents(&stateStack);
    switch(event.type) { 

    case YAML_NO_EVENT: puts("No event!"); break;
    /* Stream start/end */
    case YAML_STREAM_START_EVENT: 
        //puts("START OF STREAM"); 
    break;
    case YAML_STREAM_END_EVENT:
        //puts("END OF STREAM");   
    break;
    /* Block delimeters */
    case YAML_DOCUMENT_START_EVENT:
        printf("{"); 
    break;
    case YAML_DOCUMENT_END_EVENT:
        printf("}");
    break;
    case YAML_SEQUENCE_START_EVENT:
        Push(&stateStack, 'A');
        Push(&stateStack, 'F');
        printf("[");
    break;
    case YAML_SEQUENCE_END_EVENT:
        if (Pop(&stateStack) != 'A'){
            fprintf(stderr, "Expected to be ending an array\n");
	    exit(1);
	}
        printf("]");
        if (Peak(&stateStack) == 'V'){
	    Pop(&stateStack);
	}
    break;
    case YAML_MAPPING_START_EVENT:
        //puts("START MAP");   
        if (Peak(&stateStack) == 'V'){
	    printf("{");
	}
        if (Peak(&stateStack) == 'A'){
	    printf(", {");
	}
        Push(&stateStack, 'M');
        Push(&stateStack, 'Z');
    break;
    case YAML_MAPPING_END_EVENT:
        //puts("END MAP");   
        if (Peak(&stateStack) == 'V'){
	    Pop(&stateStack);
	    printf("}");
	}
	if (Pop(&stateStack) != 'M'){
            fprintf(stderr, "Expected to be ending a mapping\n");
            exit(1);
	}
        if (Peak(&stateStack) == 'A'){
	    printf("}");
	}
    break;
    /* Data */
    case YAML_ALIAS_EVENT:
        printf("Got alias (anchor %s)\n", event.data.alias.anchor); 
    break;
    case YAML_SCALAR_EVENT: 
        //printf("(%c)",Peak(&stateStack));
        switch(Peak(&stateStack)){
	    case 'F':
		multiLineToArray(event.data.scalar.value, 0);
                //printf("\"%s\"", event.data.scalar.value); 
		Pop(&stateStack);
	    break;
	    case 'A':
	        printf(",");
		multiLineToArray(event.data.scalar.value, 0);
                //printf(",\"%s\"", event.data.scalar.value); 
	    break;
	    case 'Z':
                printf("\"%s\" : ", event.data.scalar.value); 
		Pop(&stateStack);
                Push(&stateStack, 'V');
	    break;
	    //case 'K':
            //    printf(",\"%s\" : ", event.data.scalar.value); 
            //    Pop(&stateStack);
            //    Push(&stateStack, 'V');
	    //break;
	    case 'V':
		multiLineToArray(event.data.scalar.value, 0);
                //printf("\"%s\"", event.data.scalar.value); 
		Pop(&stateStack);
                //Push(&stateStack, 'K'); //TODO get rid of this and the K case
	    break;
	    case 'M':
                string = event.data.scalar.value; //not sure how to get around this in the best way
                if (strchr(string, newline) != NULL){ //This string has newlines!!!!
                    fprintf(stderr, "key value has newlines, can;t really make that into json.\n");
		    exit(1);
		}
                printf(", \"%s\" : ", event.data.scalar.value); 
                Push(&stateStack, 'V');
	    break;
	    default:
                printf("How the hell, we are in the default? Stack State = (%c)\n", Peak(&stateStack)); 
		exit(1);
	    break;
        }
	break;
    }
    if(event.type != YAML_STREAM_END_EVENT)
      yaml_event_delete(&event);
  } while(event.type != YAML_STREAM_END_EVENT);
  yaml_event_delete(&event);

  /* Cleanup */
  yaml_parser_delete(&parser);
  fclose(fh);

  printf("\n");
  return 0;
}
