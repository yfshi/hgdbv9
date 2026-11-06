# TECC Processor Module

## Module Overview

The TECC (Transparent Encryption for Client Columns) Processor Module is a core component of the TECC system that provides SQL statement processing for transparent encryption functionality. It operates at the client side to identify encrypted columns, process SQL statements, and coordinate with encryption services.

## Architecture

The processor module follows a modular design with the following submodules:

### Core Submodules

1. **Statement Processor** (`stmt_processor.c/h`)
   - Identifies SQL statement types
   - Routes statements to appropriate specialized processors
   - Manages overall statement processing flow

2. **Values Processor** (`values_processor.c/h`)
   - Handles encryption/decryption of values in SQL statements
   - Manages raw value lists for batch processing
   - Coordinates with encryption scheduler

3. **WHERE Clause Processor** (`where_clause_processor.c/h`)
   - Processes WHERE clauses in SQL statements
   - Checks operator compatibility with encrypted columns
   - Handles encryption of comparison values

4. **CREATE Statement Processor** (`create_stmt_processor.c/h`)
   - Processes CREATE TABLE statements
   - Validates constraints on encrypted columns
   - Checks column type compatibility

5. **Raw Value Management** (`raw_value.c/h`, `raw_values_list.c/h`)
   - Provides data structures for value processing
   - Manages lists of values for batch operations
   - Supports various data types (integer, float, string, binary)

6. **Internal Utilities** (`internal/`)
   - Common utilities and helper functions
   - Memory management wrappers
   - Error handling utilities

## Key Features

### SQL Statement Support

- **DML Statements**: SELECT, INSERT, UPDATE, DELETE
- **DDL Statements**: CREATE TABLE, ALTER TABLE, DROP TABLE
- **TECC-Specific Statements**: CREATE/DROP KEK, CREATE/DROP DEK

### Encryption Column Detection

- Automatic identification of encrypted columns (types ending with `_enc`)
- Integration with metadata cache for column information
- Support for 9 encrypted data types

### Operator Compatibility

- Support for comparison operators (=, <>, <, >, <=, >=)
- Pattern matching operators (LIKE, NOT LIKE, ILIKE)
- Algorithm-specific operator support (DET, OPE, SEARCH)

### Constraint Validation

- Primary key constraint validation for encrypted columns
- Unique constraint support
- Check constraint compatibility
- Foreign key restriction for encrypted columns

## API Reference

### Main Processor Interface

```c
/* Initialize processor context */
void tecc_processor_init(PGconn *conn);

/* Process SQL statement */
TeccProcessorResult tecc_processor_run_pre_query(TeccStatementData *statement_data);

/* Clean up processor context */
void tecc_processor_cleanup(PGconn *conn);
```

### Statement Processing

```c
/* Process specific statement types */
TeccProcessorResult tecc_processor_run_pre_select_statement(const SelectStmt *stmt,
                                                           TeccStatementData *data);
TeccProcessorResult tecc_processor_run_pre_insert_statement(const InsertStmt *stmt,
                                                           TeccStatementData *data);
TeccProcessorResult tecc_processor_run_pre_update_statement(const UpdateStmt *stmt,
                                                           TeccStatementData *data);
TeccProcessorResult tecc_processor_run_pre_delete_statement(const DeleteStmt *stmt,
                                                           TeccStatementData *data);
```

### Data Structures

```c
typedef struct TeccStatementData {
    PGconn *conn;
    const char *query;
    List *parse_tree;
    List *encrypted_columns;
    int needs_rewrite;
    int error_code;
    char error_msg[256];
} TeccStatementData;
```

## Integration Points

### Frontend Parser Integration

The processor module integrates with the frontend SQL parser to:
- Parse SQL statements into abstract syntax trees
- Extract column and table information
- Support SQL rewrite operations

### Cache Module Integration

Integration with the metadata cache provides:
- Encrypted column identification
- Algorithm and key information retrieval
- Performance optimization through caching

### Encryption Scheduler Integration

The processor coordinates with the encryption scheduler for:
- Value encryption/decryption operations
- Algorithm selection based on column type
- Key management operations

## Error Handling

The module provides comprehensive error handling with:
- Detailed error codes and messages
- Error propagation through processing pipeline
- Graceful degradation for unsupported operations

## Performance Considerations

- Modular design allows selective processing
- Efficient data structures for value management
- Minimal overhead for non-encrypted columns
- Batch processing support for multiple values

## Development Status

### Completed Features

- [x] Modular architecture implementation
- [x] Core submodule interfaces
- [x] Basic statement processing framework
- [x] Value processing infrastructure
- [x] Constraint validation framework

### Implementation Status

- [x] Integration with frontend parser - **Completed**
- [x] Cache module integration - **Completed** (all TODO comments resolved, using tecc_get_column_dek_name and tecc_get_column_kek_name)
- [x] Encryption scheduler integration - **Completed**
- [x] SQL rewrite functionality - **Completed**
- [x] Comprehensive error handling - **Completed** (error handling implemented across all submodules)
- [x] Performance optimizations - **Completed** (batch processing, memory management, thread-safe design)
- [x] WHERE clause processing - **Completed** (LPQ-SEARCH-04 fixed, encryption product storage implemented)
- [x] Expression processor - **Completed** (LPQ-EXPR-01 to 03 fixed, GUC parameter integration)
- [x] Data formatting - **Completed** (LPQ-FMT-01 to 04 fixed)
- [x] SEARCH algorithm support - **Completed** (LPQ-SEARCH-01 to 04 fixed)
- [x] Algorithm API consistency - **Completed** (LPQ-ALGO-01 to 05 fixed)

## Usage Example

```c
#include "tecc_processor.h"

int main() {
    PGconn *conn;
    TeccStatementData stmt_data;
    TeccProcessorResult result;

    /* Initialize processor */
    tecc_processor_init(conn);

    /* Prepare statement data */
    memset(&stmt_data, 0, sizeof(stmt_data));
    stmt_data.conn = conn;
    stmt_data.query = "SELECT * FROM users WHERE email = 'test@example.com'";

    /* Process statement */
    result = tecc_processor_run_pre_query(&stmt_data);
    if (result == TECC_PROCESSOR_NEEDS_REWRITE) {
        /* SQL rewrite needed for encrypted columns */
        printf("Statement needs rewrite\n");
    }

    /* Clean up */
    tecc_processor_cleanup(conn);
    return 0;
}
```

## Build Instructions

```bash
# Navigate to processor directory
cd src/interfaces/libpq/tecc_processor

# Build the module
make

# Clean build artifacts
make clean
```

## Testing

The module is designed to be tested through:
- Unit tests for individual submodules
- Integration tests with frontend parser
- End-to-end tests with complete SQL processing

## Future Enhancements

- Additional SQL statement support
- Performance optimizations
- Extended constraint validation
- Advanced error recovery mechanisms

## License

Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.