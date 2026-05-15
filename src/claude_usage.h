#ifndef CLAUDE_USAGE_H
#define CLAUDE_USAGE_H

#include <Arduino.h>

struct ClaudeUsage {
    int  session_pct;
    int  session_reset_min;
    int  weekly_pct;
    int  weekly_reset_min;
    bool ok;
    unsigned long last_updated;
};

extern ClaudeUsage gClaudeUsage;

void startClaudeUsageTask(void);

#endif // CLAUDE_USAGE_H
