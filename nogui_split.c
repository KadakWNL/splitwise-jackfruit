#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

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
    char date[16]; // DD-MM-YYYY
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
    char date[16]; // DD-MM-YYYY
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

int is_valid_date(const char *date) {
    if (strlen(date) != 10) return 0;
    if (!isdigit(date[0]) || !isdigit(date[1]) || date[2] != '-' ||
        !isdigit(date[3]) || !isdigit(date[4]) || date[5] != '-' ||
        !isdigit(date[6]) || !isdigit(date[7]) || !isdigit(date[8]) || !isdigit(date[9]))
        return 0;
    int day = atoi(date);
    int month = atoi(date + 3);
    int year = atoi(date + 6);
    if (day < 1 || day > 31) return 0;
    if (month < 1 || month > 12) return 0;
    if (year < 1) return 0;
    return 1;
}

int parse_member_ids(char *s, int *ids) {
    int count = 0;
    char *tok = strtok(s, ",");
    while (tok && count < MAX_USERS) {
        ids[count++] = atoi(trim(tok));
        tok = strtok(NULL, ",");
    }
    return count;
}

void save_data(const char *filename) {
    FILE *f = fopen(filename, "w");
    int i, j;
    for (i = 0; i < num_users; i++)
        fprintf(f, "USER|%d|%s\n", users[i].id, users[i].name);

    for (i = 0; i < num_groups; i++) {
        fprintf(f, "GROUP|%d|%s|", groups[i].id, groups[i].name);
        for (j = 0; j < groups[i].member_count; j++)
            fprintf(f, "%d%s", groups[i].member_ids[j], (j+1==groups[i].member_count?"\n":","));
    }

    for (i = 0; i < num_expenses; i++)
        fprintf(f, "EXPENSE|%d|%d|%d|%.2lf|%s|%s|%s|%s\n", expenses[i].id, expenses[i].group_id, expenses[i].paid_by_user_id,
                expenses[i].amount, expenses[i].description, expenses[i].date, expenses[i].split_type, expenses[i].category);

    for (i = 0; i < num_splits; i++)
        fprintf(f, "SPLIT|%d|%d|%.2lf\n", splits[i].expense_id, splits[i].user_id, splits[i].amount);

    for (i = 0; i < num_settlements; i++)
        fprintf(f, "SETTLEMENT|%d|%d|%d|%.2lf|%d|%s\n", settlements[i].id, settlements[i].payer_id, settlements[i].receiver_id,
                settlements[i].amount, settlements[i].group_id, settlements[i].date);
    fclose(f);
}

void load_data(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) return;
    char line[MAX_LINE];
    while (fgets(line, sizeof(line), f)) {
        char *type = strtok(line, "|");
        if (!type) continue;
        if (strcmp(type, "USER") == 0 && num_users < MAX_USERS) {
            int id; char name[64];
            char *idstr = strtok(NULL, "|"), *namestr = strtok(NULL, "\n");
            if (idstr && namestr) {
                id = atoi(trim(idstr));
                strncpy(name, trim(namestr), 63);
                users[num_users++] = (User){id, ""};
                strncpy(users[num_users-1].name, name, 63);
            }
        } else if (strcmp(type, "GROUP") == 0 && num_groups < MAX_GROUPS) {
            int id; char name[64], members[256];
            char *idstr = strtok(NULL, "|");
            char *namestr = strtok(NULL, "|");
            char *membersstr = strtok(NULL, "\n");
            if (idstr && namestr && membersstr) {
                id = atoi(trim(idstr));
                strncpy(name, trim(namestr), 63);
                strncpy(members, trim(membersstr), 255); members[255]=0;
                groups[num_groups].id = id;
                strncpy(groups[num_groups].name, name, 63);
                groups[num_groups].member_count = parse_member_ids(members, groups[num_groups].member_ids);
                num_groups++;
            }
        } else if (strcmp(type, "EXPENSE") == 0 && num_expenses < MAX_EXPENSES) {
            int id, gid, paid_by;
            double amt;
            char desc[128], date[16], stype[16], cat[32];
            char *idstr = strtok(NULL, "|"), *gidstr = strtok(NULL, "|"), *paidbystr = strtok(NULL, "|"),
                 *amtstr = strtok(NULL, "|"), *descstr = strtok(NULL, "|"), *datestr = strtok(NULL, "|"),
                 *stypestr = strtok(NULL, "|"), *catstr = strtok(NULL, "\n");
            if (idstr && gidstr && paidbystr && amtstr && descstr && datestr && stypestr && catstr) {
                id = atoi(trim(idstr));
                gid = atoi(trim(gidstr));
                paid_by = atoi(trim(paidbystr));
                amt = atof(trim(amtstr));
                strncpy(desc, trim(descstr), 127);
                strncpy(date, trim(datestr), 15);
                strncpy(stype, trim(stypestr), 15);
                strncpy(cat, trim(catstr), 31);
                expenses[num_expenses++] = (Expense){id, gid, paid_by, amt, "", "", "", ""};
                strncpy(expenses[num_expenses-1].description, desc, 127);
                strncpy(expenses[num_expenses-1].date, date, 15);
                strncpy(expenses[num_expenses-1].split_type, stype, 15);
                strncpy(expenses[num_expenses-1].category, cat, 31);
            }
        } else if (strcmp(type, "SPLIT") == 0 && num_splits < MAX_SPLITS) {
            int eid, uid; double amt;
            char *eidstr = strtok(NULL, "|"), *uidstr = strtok(NULL, "|"), *amtstr = strtok(NULL, "\n");
            if (eidstr && uidstr && amtstr) {
                eid = atoi(trim(eidstr));
                uid = atoi(trim(uidstr));
                amt = atof(trim(amtstr));
                splits[num_splits++] = (Split){eid, uid, amt};
            }
        } else if (strcmp(type, "SETTLEMENT") == 0 && num_settlements < MAX_SETTLEMENTS) {
            int id, payer, recv, gid;
            double amt;
            char date[16];
            char *idstr = strtok(NULL, "|"), *payerstr = strtok(NULL, "|"), *recvstr = strtok(NULL, "|"),
                 *amtstr = strtok(NULL, "|"), *gidstr = strtok(NULL, "|"), *datestr = strtok(NULL, "\n");
            if (idstr && payerstr && recvstr && amtstr && gidstr && datestr) {
                id = atoi(trim(idstr));
                payer = atoi(trim(payerstr));
                recv = atoi(trim(recvstr));
                amt = atof(trim(amtstr));
                gid = atoi(trim(gidstr));
                strncpy(date, trim(datestr), 15);
                settlements[num_settlements++] = (Settlement){id, payer, recv, amt, gid, ""};
                strncpy(settlements[num_settlements-1].date, date, 15);
            }
        }
    }
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
    if (num_users >= MAX_USERS) {
        printf("User limit reached!\n");
        return;
    }
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
    if (num_groups >= MAX_GROUPS) {
        printf("Group limit reached!\n");
        return;
    }
    char name[64], members[256];
    int ids[MAX_USERS], count;
    printf("Enter group name: ");
    fgets(name, sizeof(name), stdin);
    strcpy(name, trim(name));
    if(strlen(name)==0) {
        printf("Group name cannot be empty.\n");
        return;
    }
    for(int i=0; i<num_groups; ++i) {
        if(strcmp(groups[i].name, name)==0) {
            printf("Group with this name already exists.\n");
            return;
        }
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
    if (num_expenses >= MAX_EXPENSES) {
        printf("Expense limit reached!\n");
        return;
    }
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
    if (amt <= 0) {
        printf("Amount must be positive.\n");
        return;
    }
    printf("Description: ");
    fgets(desc, sizeof(desc), stdin); strcpy(desc, trim(desc));
    printf("Category: ");
    fgets(cat, sizeof(cat), stdin); strcpy(cat, trim(cat));
    printf("Date (DD-MM-YYYY): ");
    fgets(date, sizeof(date), stdin); strcpy(date, trim(date));
    if (!is_valid_date(date)) {
        printf("Invalid date format. Use DD-MM-YYYY.\n");
        return;
    }
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
        if (num_splits + groups[gidx].member_count > MAX_SPLITS) {
            printf("Split limit reached!\n");
            num_expenses--;
            return;
        }
        double each = amt / groups[gidx].member_count;
        for (int i = 0; i < groups[gidx].member_count; i++)
            splits[num_splits++] = (Split){eid, groups[gidx].member_ids[i], each};
    } else if (strcmp(stype, "custom") == 0) {
        double total = 0, share;
        if (num_splits + groups[gidx].member_count > MAX_SPLITS) {
            printf("Split limit reached!\n");
            num_expenses--;
            return;
        }
        for (int i = 0; i < groups[gidx].member_count; i++) {
            printf("Enter amount for %s: ", user_name(groups[gidx].member_ids[i]));
            scanf("%lf", &share); getchar();
            if(share < 0) {
                printf("Amount must be non-negative.\n");
                i--; continue;
            }
            splits[num_splits++] = (Split){eid, groups[gidx].member_ids[i], share};
            total += share;
        }
        if (total != amt) {
            printf("Error: Custom split does not sum to total amount! Expense not added.\n");
            num_expenses--;
            num_splits -= groups[gidx].member_count;
            return;
        }
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
    int gidx = find_group_index(group_id);
    if (gidx == -1) { printf("Group not found.\n"); return; }
    int mcount = groups[gidx].member_count;
    double balance[MAX_USERS] = {0};

    for (int i = 0; i < mcount; i++) {
        int uid = groups[gidx].member_ids[i];
        for (int j = 0; j < num_expenses; j++)
            if (expenses[j].group_id == group_id && expenses[j].paid_by_user_id == uid)
                balance[i] += expenses[j].amount;
        for (int j = 0; j < num_splits; j++)
            if (splits[j].user_id == uid) {
                for (int k = 0; k < num_expenses; k++)
                    if (splits[j].expense_id == expenses[k].id && expenses[k].group_id == group_id)
                        balance[i] -= splits[j].amount;
            }
        for (int j = 0; j < num_settlements; j++) {
            if (settlements[j].group_id == group_id) {
                if (settlements[j].payer_id == uid) balance[i] += settlements[j].amount;
                if (settlements[j].receiver_id == uid) balance[i] -= settlements[j].amount;
            }
        }
    }
    printf("Balances for group '%s':\n", groups[gidx].name);
    for (int i = 0; i < mcount; i++) {
        int uid = groups[gidx].member_ids[i];
        printf("  %s: %.2lf\n", user_name(uid), balance[i]);
    }
    printf("Suggested settlements:\n");
    double working[MAX_USERS];
    memcpy(working, balance, sizeof(working));
    for (int loop = 0; loop < mcount*2; loop++) {
        int min_idx = -1, max_idx = -1;
        for (int i = 0; i < mcount; i++) {
            if (min_idx == -1 || working[i] < working[min_idx])
                if (working[i] < 0) min_idx = i;
            if (max_idx == -1 || working[i] > working[max_idx])
                if (working[i] > 0) max_idx = i;
        }
        if (min_idx == -1 || max_idx == -1) break;
        double amt = -working[min_idx] < working[max_idx] ? -working[min_idx] : working[max_idx];
        if (amt < 0.01) break;
        printf("  %s pays %s: %.2lf\n", user_name(groups[gidx].member_ids[min_idx]),
                                         user_name(groups[gidx].member_ids[max_idx]), amt);
        working[min_idx] += amt;
        working[max_idx] -= amt;
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
    if (num_settlements >= MAX_SETTLEMENTS) {
        printf("Settlement limit reached!\n");
        return;
    }
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
    if (amt <= 0) {
        printf("Settlement amount must be positive.\n");
        return;
    }
    printf("Enter date (DD-MM-YYYY): ");
    char date[16]; fgets(date, sizeof(date), stdin); strcpy(date, trim(date));
    if (!is_valid_date(date)) {
        printf("Invalid date format. Use DD-MM-YYYY.\n");
        return;
    }
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

// Shreyas
int main() {
    load_data(DATA_FILE); // Shreyas
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
            case 1: add_user_interactive(); break; // Shavanti
            case 2: add_group_interactive(); break; // Shavanti
            case 3: add_remove_user_group(); break;  // Shreyas
            case 4: add_expense_interactive(); break; // Siddhant
            case 5: print_users(); break; // Shavanti
            case 6: print_groups(); break; // Skanda
            case 7: group_expenses_menu(); break; // Siddhant
            case 8: balances_menu(); break; // Siddhant
            case 9: settlements_menu(); break; // Skanda
            case 10: settlements_history_menu(); break; // Skanda
            case 0: save_data(DATA_FILE); printf("Bye!\n"); return 0;
            default: printf("Invalid choice\n");
        }
        save_data(DATA_FILE); // Shreyas
    }
}