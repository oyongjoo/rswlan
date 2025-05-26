# Contributing Guidelines

This document describes the development standards and guidelines for contributing to the WiFi driver project.

## Code Style Guidelines

### 1. Error Handling

#### Early Return Pattern
- Prefer early returns for error conditions
- Avoid deeply nested if-statements
- Improve code readability and maintainability
- Example:
```c
static rs_ret function_name(struct some_struct *param)
{
    rs_ret ret;
    
    ret = first_operation();
    if (ret != RS_SUCCESS)
        return ret;
        
    ret = second_operation();
    if (ret != RS_SUCCESS)
        return ret;
        
    return RS_SUCCESS;
}
```

#### Error Propagation
- Return error codes immediately when detected
- Preserve original error codes when possible
- Avoid masking errors with generic failure codes
- Use meaningful error codes that help with debugging

#### Resource Cleanup
- Use early returns only after proper cleanup
- Consider using goto for complex cleanup scenarios
- Keep cleanup code paths clear and well-documented
- Always pair resource allocation with deallocation

### 2. Function Design

#### Function Length
- Keep functions focused and concise
- Aim for functions that fit on one screen (< 40 lines)
- Break down complex functions into smaller, focused ones
- Each function should do one thing well

#### Parameter Validation
- Validate all function parameters at the beginning
- Use assertions for internal function invariants
- Document parameter requirements in function comments
- Return appropriate error codes for invalid parameters

#### Function Comments
- Document function purpose and behavior
- Describe parameters and return values
- Note any side effects or special conditions
- Include usage examples for complex functions

### 3. Variable Naming

#### General Rules
- Use descriptive, self-documenting names
- Follow the project's existing naming conventions
- Use consistent prefixes for different types
- Avoid abbreviations unless widely understood

#### Naming Conventions
- Functions: `rs_<module>_<action>`
- Variables: `snake_case`
- Constants: `UPPER_CASE`
- Types: `rs_<module>_<type>_t`

### 4. Memory Management

#### Allocation
- Check all memory allocation results
- Use appropriate allocation functions
- Consider alignment requirements
- Document ownership transfer

#### Deallocation
- Free resources in reverse order of allocation
- Null pointers after freeing
- Use designated cleanup functions
- Handle partial initialization failures

### 5. Concurrency

#### Locking
- Document lock ordering rules
- Keep critical sections small
- Avoid nested locks when possible
- Use appropriate lock types

#### Shared Resources
- Clearly document ownership rules
- Use atomic operations when appropriate
- Protect shared data consistently
- Consider lock-free alternatives

## Git Commit Guidelines

### 1. Commit Messages
- Use imperative mood in commit messages
- Start with a type prefix (feat:, fix:, docs:, etc.)
- Provide a clear, concise description
- Reference related issues or PRs

### 2. Commit Size
- Make small, focused commits
- Keep related changes together
- Separate unrelated changes
- Make commits logically complete

### 3. Branch Management
- Use feature branches for development
- Keep branches up to date with main
- Delete branches after merging
- Use meaningful branch names

## Documentation

### 1. Code Comments
- Explain why, not what
- Keep comments up to date
- Document non-obvious behavior
- Use consistent comment style

### 2. Technical Documentation
- Update relevant documentation with code changes
- Keep README.md current
- Document architectural decisions
- Include examples where helpful
