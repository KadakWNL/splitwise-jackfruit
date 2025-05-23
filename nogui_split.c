#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_USERS 100
#define MAX_GROUPS 50
#define MAX_EXPENSES 500
#define MAX_SPLITS 2000
#define MAX_SETTLEMENTS 500
#define MAX_LINE 512
#define DATA_FILE "splitwise_data.txt"

typedef struct {
    int id;
    char name[64];
} User;

typedef struct {
    int id;
    char name[64];
    int member_ids[MAX_USERS];
    int member_count;
} Group;

typedef struct {
    int id;
    int group_id;
    int paid_by_user_id;
    double amount;
    char description[128];
    char date[16];
    char split_type[16]; // "equal", "custom"
    char category[32];
} Expense;

typedef struct {
    int expense_id;
    int user_id;
    double amount;
} Split;

typedef struct {
    int id;
    int payer_id;
    int receiver_id;
    double amount;
    int group_id;
    char date[16];
} Settlement;

User users[MAX_USERS]; int num_users = 0;
Group groups[MAX_GROUPS]; int num_groups = 0;
Expense expenses[MAX_EXPENSES]; int num_expenses = 0;
Split splits[MAX_SPLITS]; int num_splits = 0;
Settlement settlements[MAX_SETTLEMENTS]; int num_settlements = 0;

char *trim(char *str) {
    char *end;
    while (*str == ' ' || *str == '\n' || *str == '\r') str++;
    end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\n' || *end == '\r')) *end-- = 0;
    return str;
}

int parse_member_ids(char *s, int *ids) {
    int count = 0, id;
    char *tok = strtok(s, ",");
    while (tok && count < MAX_USERS) {
        id = atoi(trim(tok));
        ids[count++] = id;
        tok = strtok(NULL, ",");
    }
    return count;
}

void load_data(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) return;
    char line[MAX_LINE], section[32] = "";
    while (fgets(line, sizeof(line), f)) {
        char *t = trim(line);
        if (strlen(t) == 0) continue;
        if (t[0] == '[') {
            sscanf(t, "[%31[^]]", section);
            continue;
        }
        if (strcmp(section, "USERS") == 0) {
            int id; char name[64];
            if (sscanf(t, "%d|%63[^\n]", &id, name) == 2) {
                users[num_users++] = (User){id, ""};
                strncpy(users[num_users - 1].name, trim(name), 63);
            }
        } else if (strcmp(section, "GROUPS") == 0) {
            int id; char name[64], members[256];
            if (sscanf(t, "%d|%63[^|]|%255[^\n]", &id, name, members) == 3) {
                groups[num_groups].id = id;
                strncpy(groups[num_groups].name, trim(name), 63);
                char m_copy[256]; strncpy(m_copy, members, 255);
                groups[num_groups].member_count = parse_member_ids(m_copy, groups[num_groups].member_ids);
                num_groups++;
            }
        } else if (strcmp(section, "EXPENSES") == 0) {
            int id, gid, paid_by; double amt; char desc[128], date[16], stype[16], cat[32];
            int n = sscanf(t, "%d|%d|%d|%lf|%127[^|]|%15[^|]|%15[^|]|%31[^\n]", &id, &gid, &paid_by, &amt, desc, date, stype, cat);
            if (n >= 7) {
                expenses[num_expenses++] = (Expense){id, gid, paid_by, amt, "", "", "", ""};
                strncpy(expenses[num_expenses-1].description, trim(desc), 127);
                strncpy(expenses[num_expenses-1].date, trim(date), 15);
                strncpy(expenses[num_expenses-1].split_type, trim(stype), 15);
                if (n == 8) strncpy(expenses[num_expenses-1].category, trim(cat), 31);
            }
        } else if (strcmp(section, "SPLITS") == 0) {
            int eid, uid; double amt;
            if (sscanf(t, "%d|%d|%lf", &eid, &uid, &amt) == 3)
                splits[num_splits++] = (Split){eid, uid, amt};
        } else if (strcmp(section, "SETTLEMENTS") == 0) {
            int id, payer, recv, gid; double amt; char date[16];
            if (sscanf(t, "%d|%d|%d|%lf|%d|%15s", &id, &payer, &recv, &amt, &gid, date) == 6)
                settlements[num_settlements++] = (Settlement){id, payer, recv, amt, gid, ""};
        }
    }
    fclose(f);
}

void save_data(const char *filename) {
    FILE *f = fopen(filename, "w");
    int i, j;
    fprintf(f, "[USERS]\n");
    for (i = 0; i < num_users; i++)
        fprintf(f, "%d|%s\n", users[i].id, users[i].name);

    fprintf(f, "\n[GROUPS]\n");
    for (i = 0; i < num_groups; i++) {
        fprintf(f, "%d|%s|", groups[i].id, groups[i].name);
        for (j = 0; j < groups[i].member_count; j++)
            fprintf(f, "%d%s", groups[i].member_ids[j], (j+1==groups[i].member_count?"\n":","));
    }

    fprintf(f, "\n[EXPENSES]\n");
    for (i = 0; i < num_expenses; i++)
        fprintf(f, "%d|%d|%d|%.2lf|%s|%s|%s|%s\n", expenses[i].id, expenses[i].group_id, expenses[i].paid_by_user_id,
                expenses[i].amount, expenses[i].description, expenses[i].date, expenses[i].split_type, expenses[i].category);

    fprintf(f, "\n[SPLITS]\n");
    for (i = 0; i < num_splits; i++)
        fprintf(f, "%d|%d|%.2lf\n", splits[i].expense_id, splits[i].user_id, splits[i].amount);

    fprintf(f, "\n[SETTLEMENTS]\n");
    for (i = 0; i < num_settlements; i++)
        fprintf(f, "%d|%d|%d|%.2lf|%d|%s\n", settlements[i].id, settlements[i].payer_id, settlements[i].receiver_id,
                settlements[i].amount, settlements[i].group_id, settlements[i].date);
    fclose(f);
}

const char* user_name(int id) {
    for (int i = 0; i < num_users; i++)
        if (users[i].id == id) return users[i].name;
    return "?";
}

int find_user_index(int id) {
    for (int i = 0; i < num_users; i++)
        if (users[i].id == id) return i;
    return -1;
}

int find_group_index(int id) {
    for (int i = 0; i < num_groups; i++)
        if (groups[i].id == id) return i;
    return -1;
}

void print_users() {
    printf("Users:\n");
    for (int i = 0; i < num_users; i++)
        printf("  %d: %s\n", users[i].id, users[i].name);
}

void add_user_interactive() {
    char name[64];
    printf("Enter user name: ");
    fgets(name, sizeof(name), stdin);
    strcpy(name, trim(name));
    if(strlen(name) == 0) {
        printf("Name cannot be empty.\n");
        return;
    }
    for(int i=0; i<num_users; ++i) {
        if(strcmp(users[i].name, name)==0) {
            printf("User with this name already exists.\n");
            return;
        }
    }
    users[num_users].id = num_users ? users[num_users - 1].id + 1 : 1;
    strncpy(users[num_users].name, name, 63);
    num_users++;
    printf("User added!\n");
}

void remove_user_from_group(int gid, int uid) {
    int gidx = find_group_index(gid);
    if (gidx < 0) return;
    int found = 0;
    for(int i=0; i<groups[gidx].member_count; ++i) {
        if(groups[gidx].member_ids[i]==uid) found = 1;
        if(found && i+1 < groups[gidx].member_count)
            groups[gidx].member_ids[i] = groups[gidx].member_ids[i+1];
    }
    if(found) groups[gidx].member_count--;
}

void add_group_interactive() {
    char name[64], members[256];
    int ids[MAX_USERS], count;
    printf("Enter group name: ");
    fgets(name, sizeof(name), stdin);
    strcpy(name, trim(name));
    if(strlen(name)==0) {
        printf("Group name cannot be empty.\n");
        return;
    }
    print_users();
    printf("Enter comma-separated user IDs for this group: ");
    fgets(members, sizeof(members), stdin);
    strcpy(members, trim(members));
    count = parse_member_ids(members, ids);
    if(count==0) { printf("No members specified.\n"); return; }
    groups[num_groups].id = num_groups ? groups[num_groups - 1].id + 1 : 1;
    strncpy(groups[num_groups].name, name, 63);
    groups[num_groups].member_count = count;
    for (int i = 0; i < count; i++)
        groups[num_groups].member_ids[i] = ids[i];
    num_groups++;
    printf("Group added!\n");
}

void print_groups() {
    printf("Groups:\n");
    for (int i = 0; i < num_groups; i++) {
        printf("  %d: %s (members: ", groups[i].id, groups[i].name);
        for (int j = 0; j < groups[i].member_count; j++)
            printf("%s%s", user_name(groups[i].member_ids[j]), (j+1==groups[i].member_count?"":", "));
        printf(")\n");
    }
}

void add_remove_user_group() {
    print_groups();
    printf("Enter group ID: ");
    int gid;
    scanf("%d", &gid); getchar();
    int gidx = find_group_index(gid);
    if(gidx<0) { printf("Group not found.\n"); return; }
    printf("1. Add user\n2. Remove user\nChoice: ");
    int ch;
    scanf("%d", &ch); getchar();
    if(ch==1) {
        print_users();
        printf("Enter user ID to add: ");
        int uid; scanf("%d", &uid); getchar();
        for(int i=0; i<groups[gidx].member_count; ++i)
            if(groups[gidx].member_ids[i]==uid) {
                printf("User already in group.\n"); return;
            }
        groups[gidx].member_ids[groups[gidx].member_count++] = uid;
        printf("User added to group.\n");
    } else if(ch==2) {
        printf("Enter user ID to remove: ");
        int uid; scanf("%d", &uid); getchar();
        remove_user_from_group(gid, uid);
        printf("User removed from group.\n");
    }
}

void add_expense_interactive() {
    int gid, paid_by, gidx = -1;
    double amt;
    char desc[128], date[16], stype[16], cat[32];
    print_groups();
    printf("Enter group ID: ");
    scanf("%d", &gid); getchar();
    gidx = find_group_index(gid);
    if (gidx == -1) { printf("Group not found.\n"); return; }
    printf("Who paid? Enter user ID: ");
    scanf("%d", &paid_by); getchar();
    int valid = 0;
    for(int i=0; i<groups[gidx].member_count; ++i)
        if(groups[gidx].member_ids[i]==paid_by) valid=1;
    if(!valid) { printf("User not in group.\n"); return; }
    printf("Enter amount: ");
    scanf("%lf", &amt); getchar();
    printf("Description: ");
    fgets(desc, sizeof(desc), stdin); strcpy(desc, trim(desc));
    printf("Category: ");
    fgets(cat, sizeof(cat), stdin); strcpy(cat, trim(cat));
    printf("Date (YYYY-MM-DD): ");
    fgets(date, sizeof(date), stdin); strcpy(date, trim(date));
    printf("Split type (equal/custom): ");
    fgets(stype, sizeof(stype), stdin); strcpy(stype, trim(stype));
    int eid = num_expenses ? expenses[num_expenses - 1].id + 1 : 1;
    expenses[num_expenses] = (Expense){eid, gid, paid_by, amt, "", "", "", ""};
    strncpy(expenses[num_expenses].description, desc, 127);
    strncpy(expenses[num_expenses].date, date, 15);
    strncpy(expenses[num_expenses].split_type, stype, 15);
    strncpy(expenses[num_expenses].category, cat, 31);
    num_expenses++;
    if (strcmp(stype, "equal") == 0) {
        double each = amt / groups[gidx].member_count;
        for (int i = 0; i < groups[gidx].member_count; i++)
            splits[num_splits++] = (Split){eid, groups[gidx].member_ids[i], each};
    } else if (strcmp(stype, "custom") == 0) {
        double total = 0, share;
        for (int i = 0; i < groups[gidx].member_count; i++) {
            printf("Enter amount for %s: ", user_name(groups[gidx].member_ids[i]));
            scanf("%lf", &share); getchar();
            splits[num_splits++] = (Split){eid, groups[gidx].member_ids[i], share};
            total += share;
        }
        if (total != amt)
            printf("Warning: The custom split does not sum up to the total amount!\n");
    }
    printf("Expense added!\n");
}

void print_expenses() {
    for (int i = 0; i < num_expenses; i++) {
        printf("%d: Group: %s, Paid by: %s, Amount: %.2lf, Desc: %s, Date: %s, Cat: %s, Split: %s\n",
            expenses[i].id, groups[find_group_index(expenses[i].group_id)].name, user_name(expenses[i].paid_by_user_id),
            expenses[i].amount, expenses[i].description, expenses[i].date, expenses[i].category, expenses[i].split_type);
    }
}

void print_group_expenses(int group_id) {
    for (int i = 0; i < num_expenses; i++) {
        if (expenses[i].group_id == group_id) {
            printf("%d: Paid by: %s, Amount: %.2lf, Desc: %s, Date: %s, Cat: %s, Split: %s\n",
                expenses[i].id, user_name(expenses[i].paid_by_user_id), expenses[i].amount, expenses[i].description,
                expenses[i].date, expenses[i].category, expenses[i].split_type);
        }
    }
}

void print_balances(int group_id) {
    double balance[MAX_USERS] = {0};
    int gidx = find_group_index(group_id);
    if (gidx == -1) { printf("Group not found.\n"); return; }
    for (int i = 0; i < groups[gidx].member_count; i++) {
        int uid = groups[gidx].member_ids[i];
        for (int j = 0; j < num_expenses; j++)
            if (expenses[j].group_id == group_id && expenses[j].paid_by_user_id == uid)
                balance[uid] += expenses[j].amount;
        for (int j = 0; j < num_splits; j++)
            if (splits[j].user_id == uid) {
                for (int k = 0; k < num_expenses; k++)
                    if (splits[j].expense_id == expenses[k].id && expenses[k].group_id == group_id)
                        balance[uid] -= splits[j].amount;
            }
        // Subtract settlements
        for (int j = 0; j < num_settlements; j++) {
            if (settlements[j].group_id == group_id) {
                if (settlements[j].payer_id == uid) balance[uid] -= settlements[j].amount;
                if (settlements[j].receiver_id == uid) balance[uid] += settlements[j].amount;
            }
        }
    }
    printf("Balances for group '%s':\n", groups[gidx].name);
    for (int i = 0; i < groups[gidx].member_count; i++) {
        int uid = groups[gidx].member_ids[i];
        printf("  %s: %.2lf\n", user_name(uid), balance[uid]);
    }
    printf("Suggested settlements:\n");
    double working[MAX_USERS];
    memcpy(working, balance, sizeof(working));
    for (int loop = 0; loop < groups[gidx].member_count*2; loop++) {
        int min_idx = -1, max_idx = -1;
        for (int i = 0; i < groups[gidx].member_count; i++) {
            int uid = groups[gidx].member_ids[i];
            if (min_idx == -1 || working[uid] < working[groups[gidx].member_ids[min_idx]])
                if (working[uid] < 0) min_idx = i;
            if (max_idx == -1 || working[uid] > working[groups[gidx].member_ids[max_idx]])
                if (working[uid] > 0) max_idx = i;
        }
        if (min_idx == -1 || max_idx == -1) break;
        int uid1 = groups[gidx].member_ids[min_idx], uid2 = groups[gidx].member_ids[max_idx];
        double amt = -working[uid1] < working[uid2] ? -working[uid1] : working[uid2];
        if (amt < 0.01) break;
        printf("  %s pays %s: %.2lf\n", user_name(uid1), user_name(uid2), amt);
        working[uid1] += amt;
        working[uid2] -= amt;
    }
}

void balances_menu() {
    print_groups();
    printf("Enter group ID: ");
    int gid;
    scanf("%d", &gid); getchar();
    print_balances(gid);
}

void group_expenses_menu() {
    print_groups();
    printf("Enter group ID: ");
    int gid;
    scanf("%d", &gid); getchar();
    print_group_expenses(gid);
}

void settlements_menu() {
    print_groups();
    printf("Enter group ID: ");
    int gid;
    scanf("%d", &gid); getchar();
    int gidx = find_group_index(gid);
    if(gidx<0) { printf("Group not found.\n"); return; }
    print_balances(gid);
    printf("Enter payer user ID: ");
    int payer; scanf("%d", &payer); getchar();
    printf("Enter receiver user ID: ");
    int receiver; scanf("%d", &receiver); getchar();
    printf("Enter amount: ");
    double amt; scanf("%lf", &amt); getchar();
    printf("Enter date (YYYY-MM-DD): ");
    char date[16]; fgets(date, sizeof(date), stdin); strcpy(date, trim(date));
    int sid = num_settlements ? settlements[num_settlements-1].id+1 : 1;
    settlements[num_settlements++] = (Settlement){sid, payer, receiver, amt, gid, ""};
    strncpy(settlements[num_settlements-1].date, date, 15);
    printf("Settlement recorded!\n");
}

void settlements_history_menu() {
    print_groups();
    printf("Enter group ID: ");
    int gid;
    scanf("%d", &gid); getchar();
    printf("Settlements for this group:\n");
    for(int i=0; i<num_settlements; ++i) {
        if(settlements[i].group_id == gid) {
            printf("%d: %s paid %s %.2lf on %s\n", settlements[i].id, user_name(settlements[i].payer_id),
                   user_name(settlements[i].receiver_id), settlements[i].amount, settlements[i].date);
        }
    }
}

// Sub-Prob: 8
int main() {
    load_data(DATA_FILE); // Sub-Prob: 7
    int choice;
    while (1) {
        printf("\nSplitwise CLI Menu\n"
               "1. Add User\n"
               "2. Add Group\n"
               "3. Add/Remove User from Group\n"
               "4. Add Expense\n"
               "5. Show Users\n"
               "6. Show Groups\n"
               "7. Show Expenses for Group\n"
               "8. Show Balances for Group\n"
               "9. Mark Settlement\n"
               "10. Show Settlement History\n"
               "0. Exit\n"
               "Choice: ");
        scanf("%d", &choice); getchar();
        switch (choice) {
            case 1: add_user_interactive(); break; // Sub-Prob: 1
            case 2: add_group_interactive(); break; // Sub-Prob: 1
            case 3: add_remove_user_group(); break; // Sub-Prob: 1
            case 4: add_expense_interactive(); break; // Sub-Prob: 2, 3
            case 5: print_users(); break; // Sub-Prob: 1
            case 6: print_groups(); break; // Sub-Prob: 1
            case 7: group_expenses_menu(); break; // Sub-Prob: 2, 5
            case 8: balances_menu(); break; // Sub-Prob: 4, 5
            case 9: settlements_menu(); break; // Sub-Prob: 6 (print function in this -> 4, 5)
            case 10: settlements_history_menu(); break; // Sub-Prob: 6,5
            case 0: save_data(DATA_FILE); printf("Bye!\n"); return 0; // Sub-Prob: 7
            default: printf("Invalid choice\n");
        }
        save_data(DATA_FILE);
    }
}