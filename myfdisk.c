#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
// to use something like uint8_t
#include <inttypes.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct {
	uint8_t status;
	uint8_t first_chs[3];
	uint8_t partition_type;
 	uint8_t last_chs[3];
	uint32_t lba;
	uint32_t sector_count;	
} PartitionEntry;

int main(int argc, char **argv) {
	char buf[512];
	
	//check that argc == 2
	if (argc != 2) {
		printf("Should enter './myfdisk Hard_Disk_Name'\n");
		return -1;
	}
	int fd = open(argv[1], O_RDONLY);
	if (fd < 0) {
		perror("Failed open the file:)"); 
		exit(1);
       	}	
	int read_check = read(fd, buf, 512);
	if (read_check < 0){
		perror("Failed reading from the file:)"); 
		exit(1);
	}

	if ( (( ( (char*)buf )[510]& 0xFF ) != 85) || (( ( (char*)buf )[511]& 0xFF ) != 170) ) {
		perror("The buffer is not ending with 55 aa");
		exit(1);	
	}
#if 0
	// making sure that 55(hex) = 85(dec) and aa(hex) = 170(dec)
	printf("%x\n", ((char*)buf)[510]& 0xFF);
	printf("%i\n", ((char*)buf)[510]& 0xFF);
	printf("%x\n", ((char*)buf)[511]& 0xFF);
	printf("%i\n", ((char*)buf)[511]& 0xFF);

        // printing the whole buffer values to show the bytes stored in the file
	for (int i = 0; i < 512; i++)
        {
                printf("%02x ", ((char *)buf)[i]& 0xFF);
        }
        printf("\n");
#endif

	int isExtended[4] = {0, 0, 0, 0};
	//check that read return vlues and the last two bytes are 55 aa
	PartitionEntry *table_entry_ptr = (PartitionEntry *) & buf[446];
	printf("%-5s %-10s %-10s %-10s %-10s %-10s %-10s %-10s\n", "Device", "Boot", "start", "End", "sectors", "Size", "Id", "Type");
	
	for (int i = 0; i < 4; i++) {
		printf("%s%-5d %-10c %-10u %-10u %-10u %uG	%-10X\n", 
			argv[1],
			i+1, 
			table_entry_ptr[i].status == 0x80 ? '*' : ' ',
			table_entry_ptr[i].lba,
			table_entry_ptr[i].lba + table_entry_ptr[i].sector_count - 1,
			table_entry_ptr[i].sector_count,
			(uint32_t) (((uint64_t) table_entry_ptr[i].sector_count * 512) / (1024 * 1024 * 1024)),
			table_entry_ptr[i].partition_type);
		if (table_entry_ptr[i].partition_type == 5){
			isExtended[i] = 1;
		}	
	}
	
	int absolute_start_index;
	bool connected;
	int start, end;
	int partition_count = 5;
	for (int i = 0; i < 4; i++){
		//printf("%i\n", isExtended[i]);
		if (isExtended[i] == 1){
			//printf("this is extneded\n");
			absolute_start_index = table_entry_ptr[i].lba*512;
			//printf("absolute start index = %i);
			
			lseek(fd, absolute_start_index, SEEK_SET);
			//printf("the absolute start index of %i, and divided by 512 = %i\n", absolute_start_index, absolute_start_index/512);
			read(fd, buf, 512);
			int ebr_start_index = absolute_start_index;
			connected = true;
			while (connected){
				//printf("the ebr start index is %i, and divided by 512 = %i\n", ebr_start_index, ebr_start_index/512);
				PartitionEntry *extended_entry_ptr = (PartitionEntry *) & buf[446];
				start = (ebr_start_index/512) + (extended_entry_ptr[0].lba);
				end = start + (extended_entry_ptr[0].sector_count) - 1;
				printf("%s%-5d %-10c %-10u %-10u %-10u %uG	%-10X\n", 
					argv[1],
					partition_count, 
					extended_entry_ptr[0].status == 0x80 ? '*' : ' ',
					start,
					end,
					extended_entry_ptr[0].sector_count,
					(uint32_t) (((uint64_t) extended_entry_ptr[0].sector_count * 512) / (1024 * 1024 * 1024)),
					extended_entry_ptr[0].partition_type);
				partition_count++;
				if (extended_entry_ptr[1].lba == 0 && extended_entry_ptr[1].sector_count == 0){
					connected = false;
				}
				else {
					ebr_start_index = absolute_start_index + (extended_entry_ptr[1].lba*512);
					lseek(fd, ebr_start_index, SEEK_SET);
					read(fd, buf, 512);
				}

			}
		}	
	}
	
	return 0;
}
