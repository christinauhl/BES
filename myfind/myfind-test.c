#include <sys/types.h>
#include <dirent.h>

#include <stdlib.h>
#include <stdio.h>

int main(int argc, char* argv[]) {
    
    DIR* dirp = opendir(argv[1]);
    
    if(dirp == NULL) {
        puts("Fehler: Kann Ordner nicht oeffnen");
        exit(1);
        
    }
    
    else {
        puts("OK");
    }
    
    struct dirent* file;
    
    while((file = readdir(dirp))) {
        
        printf("%s\n", file->d_name);
    }
    return 0;
}
