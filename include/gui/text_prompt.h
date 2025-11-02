#ifndef _TEXT_PROMPT
#define _TEXT_PROMPT

//  NOTE: `name` is only valid for the duration of the callback.
typedef void (*SubmitCallback)(char *name);

void text_prompt(const char *prompt_text, SubmitCallback submit_callback);

// Handle keyboard inputs for the prompt. Returns 1 if it should capture all
// inputs, that is no other inputs should be processed.
int text_prompt_handle_inputs(void);

int text_prompt_render(void);

#endif
