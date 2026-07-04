#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ── ltrace + GDB Dual Tool Workflow ──
// ltrace: see library function call arguments and return values
// GDB:    dive inside functions to see variables and execution flow

int compute_score(const char *name, int base) {
    size_t len = strlen(name);

    // GDB can see: value of len, content of name, value of base
    // ltrace can see: strlen(name) call and return value
    int score = base + (int)(len * 10);

    // Simulate: certain names get extra bonus points
    if (strstr(name, "admin")) {
        score += 100;   // GDB: score jumps from normal to +100
    }

    return score;
}

void process_names() {
    const char *users[] = {
        "alice",
        "bob_admin",
        "charlie",
        "dave_admin_super",
        "eve",
        NULL
    };

    for (int i = 0; users[i] != NULL; i++) {
        // ltrace can see: strlen(users[i]) called each time
        // GDB can see: value of i, content of users[i], returned score
        int s = compute_score(users[i], 50);
        printf("  user %-20s -> score %d\n", users[i], s);
    }
}

char *build_greeting(const char *name) {
    // ltrace: strlen(name), malloc(...), strcpy, strcat
    // GDB:    can print intermediate results at each step
    size_t nlen = strlen(name);
    char *buf = (char *)malloc(nlen + 20);

    strcpy(buf, "Hello, ");
    strcat(buf, name);
    strcat(buf, "!");

    return buf;  // ltrace: buf = 0x... (returned pointer)
                 // GDB:    print buf → "Hello, alice!"
}

int main() {
    printf("╔════════════════════════════════════╗\n");
    printf("║    ltrace + GDB Dual Perspective Debugging          ║\n");
    printf("╚════════════════════════════════════╝\n\n");

    printf("[Scenario 1] Compute Score (strlen + strstr)\n");
    process_names();

    printf("\n[Scenario 2] Build Greeting (strlen + strcpy + strcat)\n");
    char *greeting = build_greeting("Alice");
    printf("  greeting: %s\n", greeting);
    free(greeting);

    printf("\n=== Experiment End ===\n");
    return 0;
}
