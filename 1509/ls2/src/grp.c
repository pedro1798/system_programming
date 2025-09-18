# include <grp.h>
# include <stddef.h>
# include <stdio.h>

char * gid_to_name(gid_t gid) {
    /*
     * returns pointer to group number gid. used getgrgid(3)
     */

    struct group * getgrgid(), *grp_ptr;
    static char numstr[10];

    if ((grp_ptr = getgrgid(gid)) == NULL) {
        sprintf(numstr, "%d", gid);
        return numstr;
    } else {
        return grp_ptr->gr_name;
    }
}
