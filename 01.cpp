#include <iostream>
#include <Windows.h>
#include <stdio.h>
#include <conio.h>
#include <string>
#include <math.h>

#pragma warning (disable:4996)
using namespace std;

#pragma pack (1)
struct FLASH_info
{
	unsigned char start[3];
	unsigned char dos_vers[8];
	unsigned _int16 bytes_per_sector;
	unsigned _int8 sector_per_cluster;
	unsigned _int16 num_reserv_sector;
	unsigned _int8 num_of_FAT;
	unsigned _int16 num_of_dir_entries;
	unsigned _int16 LOW_total_sector;
	unsigned _int8 media_descr_type;
	unsigned _int16 num_of_sector_per_FAT_12_16;
	unsigned _int16 num_of_sector_per_track;
	unsigned _int16 num_of_heads;
	unsigned _int32 num_of_hidden_sector;
	unsigned _int32 LARGE_total_sector;

	unsigned _int32 num_of_sector_per_FAT_32;
	unsigned _int16 flags;
	unsigned _int16 FAT_version;
	unsigned _int32 cluster_num_of_root_dir;
	unsigned _int16 sector_num_FSInfo;
	unsigned _int16 sector_num_backup_boot;
	unsigned char reserved[12];
	unsigned _int8 drive_num;
	unsigned _int8 winNT_flags;
	unsigned _int8 signature;
	unsigned _int32 serial_number;
	unsigned char volum_label_string[11];
	unsigned char system_id_string[8];
	unsigned char boot_code[420];
	unsigned _int16 AA55;
};
struct FILE_info
{
	unsigned char name[8];
	unsigned char type[3];
	unsigned _int8 attr;
	unsigned _int8 WinNT;
	unsigned _int8 CrtTimeTenth;
	unsigned _int16 CrtTime;
	unsigned _int16 CrtDate;
	unsigned _int16 LstAccDate;
	unsigned _int16 FstClusHI;
	unsigned _int16 WrtTime;
	unsigned _int16 WrtDate;
	unsigned _int16 FstClusLO;
	unsigned _int32 FileSize;
};

HANDLE OpenDisk( char ch)
{
	char device_name[20];
	sprintf(device_name, "\\\\.\\%c:", ch);
	return CreateFile(device_name,									//имя
		GENERIC_ALL,
		FILE_SHARE_READ,											//дуплекс\симплекс
		NULL,														//дескриптор защиты
		OPEN_EXISTING,												//действие в случае ошибки
		FILE_ATTRIBUTE_NORMAL,										//флаг атр. файла (для девайса - NULL))
		NULL);														//шаблон
}
HANDLE OpenNewFile(int ch)
{
	char file_name[100];
	sprintf(file_name, "A:\\cluster.%d.bin", ch);
	return CreateFile(file_name,					//имя
		FILE_ALL_ACCESS,							//режим доступа
		FILE_SHARE_READ | FILE_SHARE_WRITE,			//дуплекс\симплекс
		NULL,										//дескриптор защиты
		CREATE_NEW,									//действие в случае ошибки
		0,											//флаг атр. файла (для девайса - NULL))
		NULL);										//шаблон
}

bool Read_FLASH_info(FLASH_info &fl_info, HANDLE flash);
bool Read_One_Cluster(HANDLE hdevice, int num_sector);
unsigned _int32 Get_first_cluster_of_file(HANDLE hdevice, unsigned _int32 dir_cluster, char* &file_info);
bool Read_File(HANDLE hdevice, FLASH_info flash, char* file_name);
unsigned _int32 Get_size_of_file(HANDLE hdevice, unsigned _int32 dir_cluster, char* &file_info);
char* Find_file(HANDLE hdevice, unsigned _int32 dir_cluster, char* file_name, char* &file_info);
unsigned _int32* Get_cluster_chain(HANDLE hdevice, unsigned _int32 num_reserv_sector, unsigned _int32 dir_cluster, unsigned _int32 start_cluster, unsigned _int32 num_cluster);

bool Write_File(HANDLE hdevice, FLASH_info flash, char* file_name);
unsigned _int32* Gen_cluster_chain(HANDLE hdevice, FLASH_info flash, unsigned _int32 num_cluster);
bool Gen_file_info(HANDLE hdevice, FLASH_info flash, unsigned _int32 num_cluster, unsigned _int32 start_cluster, unsigned _int32 size_file);
bool inline Reload_Dir_info(unsigned char bufer[], FILE_info file);
bool copy_sector(HANDLE hdevice, unsigned _int32 from, unsigned _int32 to, unsigned _int32 num);
unsigned _int32 Load_file_data(HANDLE hdevice, FLASH_info flash, FILE *f, unsigned _int32* cluster_chain);




int main()
{
	HANDLE hdevice = OpenDisk(_getch());
	if ((unsigned int)hdevice == 0xFFFFFFFF)
		cout << "ups";
	FLASH_info flash;

	Read_FLASH_info(flash, hdevice);
		
	//Read_File(hdevice, flash, "PASPORT ");
	Write_File(hdevice, flash, "A:\\05.jpg");

	CloseHandle(hdevice);
	
	return 0;
}

int Read_FLASH_info(FLASH_info &fl_info, HANDLE flash)
{
	DWORD Bytes_READ;
	SetFilePointer(flash, 0, NULL, FILE_BEGIN);

	unsigned char buffer[512];
	ReadFile(flash, &fl_info, 512, Bytes_READ, NULL);

	return Bytes_READ;
}
bool Read_One_Cluster(HANDLE hdevice, int num_sector)
{

	DWORD Bytes_READ;
	DWORD Bytes_WRITER;
	unsigned char buffer[512];
	SetFilePointer(hdevice, 512 * num_sector, NULL, FILE_BEGIN);
	if (ReadFile(hdevice, buffer, 512, &Bytes_READ, NULL))
	{
		HANDLE hfile = OpenNewFile(num_sector);
		WriteFile(hfile, buffer, Bytes_READ, &Bytes_WRITER, NULL);
		CloseHandle(hfile);
	}
	return 0;
}
unsigned _int32 Get_first_cluster_of_file(HANDLE hdevice,unsigned _int32 dir_cluster, char* &file_info)
{
	unsigned _int32 num_cluster = 0;
	num_cluster ^= file_info[26];
	num_cluster ^= (file_info[27]<<8);
	num_cluster ^= (file_info[20]<<16);
	num_cluster ^= (file_info[21]<<24);
	return num_cluster;
}
bool Read_File(HANDLE hdevice, FLASH_info flash, char* file_name)
{
	char * file_info = new char[32];
	unsigned _int32 dir_cluster = flash.num_reserv_sector + (2 * flash.num_of_sector_per_FAT_32);

	if (!Find_file(hdevice, dir_cluster, file_name, file_info))
	{
		delete file_info;
		return NULL;
	}
	unsigned _int32 start_cluster = Get_first_cluster_of_file(hdevice, dir_cluster, file_info);
	unsigned _int32 num_cluster = ceil((double)Get_size_of_file(hdevice, dir_cluster, file_info)/ (flash.bytes_per_sector*flash.sector_per_cluster));
	delete file_info;

	unsigned _int32* cluster_chain = new unsigned _int32[num_cluster];
	cluster_chain = Get_cluster_chain(hdevice, flash.num_reserv_sector, dir_cluster, start_cluster, num_cluster);

	DWORD Bytes_READ;
	DWORD Bytes_WRITER;
	unsigned char* buffer = new unsigned char[flash.bytes_per_sector*flash.sector_per_cluster];
	
	HANDLE hfile = OpenNewFile(2);
	for (int i = 0; i<num_cluster; i++)
	{
		Bytes_READ = 0;
		SetFilePointer(hdevice, (dir_cluster*flash.bytes_per_sector)+ (flash.bytes_per_sector *flash.sector_per_cluster* (cluster_chain[i] - 2)), NULL, FILE_BEGIN);
		ReadFile(hdevice, buffer, flash.bytes_per_sector*flash.sector_per_cluster, &Bytes_READ, NULL);
		WriteFile(hfile, buffer, Bytes_READ, &Bytes_WRITER, NULL);
	}
	CloseHandle(hfile);

	return 0;
}
unsigned _int32 Get_size_of_file(HANDLE hdevice, unsigned _int32 dir_cluster, char* &file_info)
{
	unsigned _int32 size_of_file = 0;
	size_of_file ^= file_info[0x1c];
	size_of_file ^= (file_info[0x1d] << 8);
	size_of_file ^= (file_info[0x1e] << 16);
	size_of_file ^= (file_info[0x1f] << 24);
	return size_of_file;
}
char* Find_file(HANDLE hdevice, unsigned _int32 dir_cluster, char* file_name, char* &file_info)
{
	SetFilePointer(hdevice, dir_cluster * 512, NULL, FILE_BEGIN);

	DWORD Bytes_READ;
	unsigned char buffer[512];
	unsigned _int32 num_cluster = 0;
	ReadFile(hdevice, buffer, 512, &Bytes_READ, NULL);

	for (int i = 0; i < 16; i++)
	{
		for (int j = 0; j < 32; j++)
			file_info[j] = buffer[(i * 32) + j];
		if (file_info[0] == 0xE5 || file_info[0] == 0x05)
			continue;
		if (file_info[11] == 0x0F)
			continue;

		int check = 0;
		for (short count = 0; count < 8; count++)
			if (file_name[count] != file_info[count] && file_name[count] != 0x20)
				break;
			else check++;
			if (check == 8)
				return file_info;
	}
	return NULL;
}
unsigned _int32* Get_cluster_chain(HANDLE hdevice, unsigned _int32 num_reserv_sector, unsigned _int32 dir_cluster, unsigned _int32 start_cluster, unsigned _int32 num_cluster)
{
	SetFilePointer(hdevice, 512 * num_reserv_sector+(start_cluster/128), NULL, FILE_BEGIN);
	unsigned _int32 sectors_per_fat_file = (num_cluster / 128) + 2;
	unsigned _int32* table = new unsigned _int32[128];
	unsigned _int32* chain = new unsigned _int32[num_cluster];
	unsigned _int32 chain_count = 0;
	DWORD Bytes_READ;

	for (unsigned _int32 i = 0; i < sectors_per_fat_file; i++)
	{
		ReadFile(hdevice, table, 512, &Bytes_READ, NULL);
		for (int j = 0; j < 128; j++)
		{
			chain[chain_count] = start_cluster;
			chain_count++;
			start_cluster = table[start_cluster % 128];
			if (start_cluster == 0x0FFFFFFF)
				return chain;
			if ((start_cluster % 128) == 0)
				break;
		}
	}
}

bool Write_File(HANDLE hdevice, FLASH_info flash, char* file_name)
{
	FILE *f = fopen(file_name, "rb");
	fseek(f, 0L, SEEK_END);
	unsigned _int32 size_file = ftell(f);
	unsigned _int32 num_cluster = ceil((double)size_file / (flash.sector_per_cluster*flash.bytes_per_sector));
	unsigned _int32* chain = new unsigned _int32[num_cluster]; 
	chain = Gen_cluster_chain(hdevice, flash, num_cluster);
	Gen_file_info(hdevice, flash, num_cluster, chain[0], size_file);
	Load_file_data(hdevice, flash, f, chain);
	return  0;

	
}
unsigned _int32* Gen_cluster_chain(HANDLE hdevice, FLASH_info flash, unsigned _int32 num_cluster)
{
	SetFilePointer(hdevice, flash.bytes_per_sector * flash.num_reserv_sector, NULL, FILE_BEGIN);
	unsigned _int32* table_1 = new unsigned _int32[128];
	unsigned _int32* table_2 = new unsigned _int32[128];
	unsigned _int32* chain = new unsigned _int32[num_cluster];

	DWORD Bytes_READ;
	DWORD Bytes_WRITER;
	unsigned _int32 last_cluster = 0;
	unsigned _int32 count = 0;
	bool check = false;

	for (unsigned _int32 j=0;;j++)
	{
		ReadFile(hdevice, table_1, flash.bytes_per_sector, &Bytes_READ, NULL);
		for (int i = 0; i < 128; i++)
		{
			if (table_1[i] == 0)
			{
				if ((count + 1) == num_cluster)
				{
					if (check)
						table_1[last_cluster % 128] = i + (j * 128);
					last_cluster = i+(j*128);
					chain[count] = last_cluster;
					table_1[last_cluster % 128] = 0x0FFFFFFF;

					SetFilePointer(hdevice, (-1)*flash.bytes_per_sector, NULL, FILE_CURRENT);
					WriteFile(hdevice, table_1, Bytes_READ, &Bytes_WRITER, NULL);
					delete table_1;
					delete table_2;
					return chain;
				}
				else if (count == 0)
				{
					check = true;
					last_cluster = i + (j * 128);
					chain[count] = last_cluster;
					count++;
				}
				else
				{
					table_1[last_cluster % 128] = i + (j * 128);
					last_cluster = i + (j * 128);
					chain[count] = last_cluster;
					count++;
				}
			}
		}
		if (count == num_cluster)
		{
			table_1[last_cluster % 128] = 0x0FFFFFFF;

			SetFilePointer(hdevice, (-1)*flash.bytes_per_sector, NULL, FILE_CURRENT);
			WriteFile(hdevice, table_1, Bytes_READ, &Bytes_WRITER, NULL);

			delete table_1;
			delete table_2;
			return chain;
		}
		if(check)
			for (unsigned _int32 z = 0;; z++)
			{
				Bytes_READ = 0;
				bool check_1 = false;
				ReadFile(hdevice, table_2, flash.bytes_per_sector, &Bytes_READ, NULL);
				for (int i = 0; i < 128; i++)
				{
					if (table_2[i] == 0)
					{
						table_1[last_cluster % 128] = i + ((j + z + 1) * 128);
						last_cluster = i + ((z + 1) * 128);
						chain[count] = last_cluster;
						count++;
						check_1 = true;
						break;
					}
				}
				if (check_1)
				{
					SetFilePointer(hdevice, (-1)*(flash.bytes_per_sector * (z+2)), NULL, FILE_CURRENT);
					WriteFile(hdevice, table_1, Bytes_READ, &Bytes_WRITER, NULL);
					SetFilePointer(hdevice, flash.bytes_per_sector * z, NULL, FILE_CURRENT);
					j += z;
					count--;
					break;
				}
			}
	}
}
bool Gen_file_info(HANDLE hdevice, FLASH_info flash, unsigned _int32 num_cluster, unsigned _int32 start_cluster, unsigned _int32 size_file)
{
	FILE_info file;

	gets_s((char*)file.name, 9);
	gets_s((char*)file.type, 4);
	file.attr = 0x20;
	file.WinNT = 0x01;
	file.CrtTimeTenth = 0x0D;
	file.CrtTime = 0x905D;
	file.CrtDate = 0x48AE;
	file.LstAccDate = 0x48AE;
	file.FstClusHI = ((start_cluster >> 16) & 0xFFFF);
	file.WrtTime = 0x6776;
	file.WrtDate = 0x4866;
	file.FstClusLO = ((start_cluster & 0xFFFF));
	file.FileSize = size_file;

	unsigned _int32 dir_sector = flash.bytes_per_sector * (flash.num_reserv_sector + (2 * flash.num_of_sector_per_FAT_32));
	unsigned char* dir_info = new unsigned char[flash.bytes_per_sector];
	DWORD Bytes_READ;
	DWORD Bytes_WRITER;


	SetFilePointer(hdevice, dir_sector, NULL, FILE_BEGIN);
	ReadFile(hdevice, dir_info, flash.bytes_per_sector, &Bytes_READ, NULL);

	for (int i = 0; i < 16; i++)
	{
		if (dir_info[i * 32] == 0xE5 || dir_info[i * 32] == 0x05 || dir_info[i * 32] == 0x00)
		{
			Reload_Dir_info(&dir_info[i * 32], file);
			break;
		}
	}
	SetFilePointer(hdevice, (-1)*flash.bytes_per_sector, NULL, FILE_CURRENT);
	WriteFile(hdevice, dir_info, Bytes_READ, &Bytes_WRITER, NULL);

	return 0;
}
bool inline Reload_Dir_info(unsigned char bufer[], FILE_info file)
{
	for (int i = 0; i < 32; i++)
		bufer[i] = (char &)file.name[i];
	return TRUE;
}
bool copy_sector(HANDLE hdevice, unsigned _int32 from, unsigned _int32 to, unsigned _int32 num)
{
	DWORD Bytes_READ;
	DWORD Bytes_WRITER;

	char buf[512];
	SetFilePointer(hdevice, from, NULL, FILE_BEGIN);
	ReadFile(hdevice, buf, 512, &Bytes_READ, NULL);

	SetFilePointer(hdevice, to, NULL, FILE_BEGIN);
	WriteFile(hdevice, buf, Bytes_READ, &Bytes_WRITER, NULL);

	return 0;
}
unsigned _int32 Load_file_data(HANDLE hdevice, FLASH_info flash, FILE *f, unsigned _int32* cluster_chain)
{
	DWORD Bytes_READ;
	DWORD Bytes_WRITER;
	fseek(f, 0L, SEEK_SET);

	char * buf = new char[flash.bytes_per_sector*flash.sector_per_cluster];
	unsigned _int32 size_cluster = flash.bytes_per_sector * flash.sector_per_cluster;
	unsigned _int32 zero_cluster = (flash.bytes_per_sector * (flash.num_reserv_sector + (2 * flash.num_of_sector_per_FAT_32))) - (2*size_cluster);
	unsigned _int32 i = 0,
				 step = 0;

	while(!feof(f))
	{
		Bytes_READ = 0;
		Bytes_READ = fread(buf, 1, flash.bytes_per_sector*flash.sector_per_cluster, f);
		step = cluster_chain[i] * flash.bytes_per_sector*flash.sector_per_cluster;

		SetFilePointer(hdevice, zero_cluster+step, NULL, FILE_BEGIN);
		WriteFile(hdevice, buf, 2048, &Bytes_WRITER, NULL);
		i++;
	}
	return 0;
}