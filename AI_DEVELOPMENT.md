# Usage of AI Tools

This document outlines how AI tools were leveraged during the development of this project.

### AI Models Used :
- **Claude Haiku 4.5** (Agent Mode) - Used for complex, multi-step tasks (also this model provides very fast response time);
- **GPT 5.0** - Used for general questions, quick solutions.

### Other resources
- **Google** - Used for the hardest technical questions, primarily referencing:
  - [Stack Overflow](https://stackoverflow.com) 
  - [man7.org](https://man7.org)

### Development Methodology

#### Planning Phase
Before generating code for complex features, a planning phase was executed: AI analyzed the requirements and created a todo list with clear implementation steps. Important to modify this plan before code generation so it would be aligned with your objectives.


#### 1 Feature >= 1 Prompt
"One feature >= One prompt" (a.k.a "do not implement 112462 features at once") principle was followed . This allows not to dilute the context of the model and simplify the debugging process.

#### Context Management
Important to include related files to the context of the model in order to accelerate the output and don't mislead the agent in refencing a definition/declation of a specific element.
