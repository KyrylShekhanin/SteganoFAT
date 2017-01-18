#include <iostream>
#include <Windows.h>
#include <ctime> /*add to text */
#include <conio.h>
#include <math.h>

#pragma warning (disable:4996)
using namespace std;

#pragma pack (1)
struct FLASH_INFO
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
struct FILE_INFO
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

HANDLE OpenDisk(char ch)
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
int ReadFlasfInfo(FLASH_INFO &f_info, HANDLE flash);
void quickSort(unsigned _int32 arr[], int left, int right);
void quickSort_2(unsigned _int32* arr[], int left, int right);

FILE_INFO* ChoiceOfFiles(HANDLE fdevice, FLASH_INFO f_info, int num_files);
/*For ChoiceFile*/
inline char* CorectName(char* file_name);
inline int InputFileNames(char* &file_name);

int WriteMesage(HANDLE fdevice, FLASH_INFO f_info, FILE_INFO* files, int num_files);
/*For Write*/
inline _int32 MinimumSizeFile(FILE_INFO* files, int num_files);
FILE * FileOpenToRead();
inline int SizeFile(FILE *f);
char* CreateInformArray(char* mes, int size_file, int MASK);
inline unsigned _int32 GetFirstClusterOfFile(FILE_INFO files);
unsigned _int32* GetClusterChain(HANDLE hdevice, FLASH_INFO f_info, unsigned _int32 start_cluster, unsigned _int32 num_cluster);
char ** LoadDataFilesToBuf(HANDLE hdevice, FLASH_INFO f_info, unsigned _int32** cluster_chain, unsigned _int32* num_cluster, unsigned _int32 num_files);
unsigned _int32** ChainTransformation(unsigned _int32** elder_cluster_chain, unsigned _int32* &start_cluster, unsigned _int32* num_cluster,
	int num_files, char* stegano_mesage, unsigned _int32 size_stegano_mesage);
unsigned _int32 LoadDataFilesFromBuf(HANDLE hdevice, FLASH_INFO f_info, unsigned _int32** cluster_chain, unsigned _int32* num_cluster, 
	unsigned _int32 num_files, char** bufer_for_file_data);
unsigned _int32 LoadClusterChainToFat(unsigned _int32** cluster_chain, HANDLE hdevice, FLASH_INFO f_info, unsigned _int32* num_cluster, unsigned _int32 num_files);
int ReloadFileInfo(HANDLE fdevice, FLASH_INFO f_info, FILE_INFO* files, int num_files, unsigned _int32* start_cluster);

int ReadMesage(HANDLE fdevice, FLASH_INFO f_info, FILE_INFO* files, int num_files);
/*For Read*/
FILE* FileOpenToWrite();
char* ReadSteaganoFromClusterChain(unsigned _int32** cluster_chain, unsigned _int32* num_cluster, unsigned _int32 num_files,
	unsigned _int32 size_stegano_mesage);
char* CreateMesageFromStg(char* stg, int size_stg, int MASK);


int main()
{
	HANDLE fdevice;
	FLASH_INFO f_info;
	do {
		cin.clear();
		system("cls");
		cout << "Name of the valume: ";
		char ch;
		scanf("%c", &ch);
		fdevice = OpenDisk(ch);
		if ((int)fdevice == 0xFFFFFFFF)
		{
			cout << "\nInvalid name | Stop reading processes (close)\nPress 'q' to quit\n";
			if (_getch() == 'q')
				return 0;
		}
	} while (((int)fdevice == 0xFFFFFFFF));
	ReadFlasfInfo(f_info, fdevice);



	int num_of_files;
	cout << "Entry number of cover files: ";
	scanf("%i", &num_of_files);
	FILE_INFO* files;
	files = ChoiceOfFiles(fdevice, f_info, num_of_files);

	if (files)
	{
		
		printf("\nr - Read message from FAT.\n");
		printf("w - Write message to FAT.\n");
		printf("Make choice: ");
		char ch = _getche();
		switch (ch)
		{
		case 'r':
			ReadMesage(fdevice, f_info, files, num_of_files);
			break;
		case 'w':
			WriteMesage(fdevice, f_info, files, num_of_files);
			break;
		}
		printf("\nEND");
	}
	CloseHandle(fdevice);
	_getch();
	return 0;
}

int ReadFlasfInfo(FLASH_INFO &fl_info, HANDLE flash)
{
	DWORD Bytes_READ;

	SetFilePointer(flash, 0, NULL, FILE_BEGIN);
	ReadFile(flash, &fl_info, 512, &Bytes_READ, NULL);
	SetFilePointer(flash, 0, NULL, FILE_BEGIN);

	return Bytes_READ;
}
FILE_INFO* ChoiceOfFiles(HANDLE fdevice, FLASH_INFO flash_info, int num_files)
{
		
		
	FILE_INFO* files = new FILE_INFO[num_files];
	char** file_name = new char*[num_files]; /*delete DONE*/
	for (int i = 0; i < num_files; i++)
		file_name[i] = new char[8]; /*delete DONE*/
	printf("Print file names:\n");
	while (fgetc(stdin) != '\n');
	for (int i = 0; i < num_files; i++)
	{
		printf("%i)",i);
		if (InputFileNames(file_name[i]) == -1)
			return 0;
	}

	unsigned _int32 dir_sector = flash_info.num_reserv_sector + (flash_info.num_of_FAT * flash_info.num_of_sector_per_FAT_32);
	char* buffer = new char[flash_info.bytes_per_sector*flash_info.sector_per_cluster]; /*delete DONE*/
	DWORD Bytes_READ;
	SetFilePointer(fdevice, dir_sector * flash_info.bytes_per_sector, NULL, FILE_BEGIN);
	ReadFile(fdevice, buffer, flash_info.bytes_per_sector*flash_info.sector_per_cluster, &Bytes_READ, NULL);
		
	for (int x = 0; x < num_files; x++)
	{
		bool global_true_name = false;
		for (int i = 0; i < 16 * flash_info.sector_per_cluster; i++)
		{
			bool true_name = false;
			for (int j = 0; j < 32; j++)
				files[x].name[j] = buffer[(i * 32) + j];
			if (files[x].name[0] == 0xE5 || files[x].name[0] == 0x05)
				continue;
			if (files[x].name[11] == 0x0F)
				continue;
			for (short count = 0; count < 8; count++)
			{
				if (file_name[x][count] != files[x].name[count] && files[x].name[count] != 0x20)
				{
					true_name = false;
					break;
				}
				else true_name = true;
			}
			if (true_name)
			{
				global_true_name = true;
				break;
			}
				
		}
		if(global_true_name)
			printf("File information #%i was found\n", x);
		else {
			files = NULL;
			printf("File name #%i invalid\n", x);
		}
	}

	for (int i = 0; i < num_files; i++)
		delete[]file_name[i]; /* file_name[i]*/
	delete[]file_name;/* file_name*/
	delete[]buffer;/*bufer*/

	return files;
}
void quickSort(unsigned _int32 arr[], int left, int right)
{
	unsigned _int32 i = left, j = right;
	unsigned _int32 tmp;
	unsigned _int32 pivot = arr[(left + right) / 2];

	/* partition */
	while (i <= j) {
		while (arr[i] < pivot)
			i++;
		while (arr[j] > pivot)
			j--;
		if (j == 0) j++;
		if (i <= j) {
			tmp = arr[i];
			arr[i] = arr[j];
			arr[j] = tmp;
			i++;
			j--;
		}
	};

	/* recursion */
	if (left < j)
		quickSort(arr, left, j);
	if (i < right)
		quickSort(arr, i, right);

}
void quickSort_2(unsigned _int32* arr[], int left, int right)
{
	unsigned _int32 i = left, j = right;
	unsigned _int32 tmp_1, tmp_2;
	unsigned _int32 pivot = arr[(left + right) / 2][0];

	/* partition */
	while (i <= j) {
		while (arr[i][0] < pivot)
			i++;
		while (arr[j][0] > pivot)
			j--;
		if (j == 0) j++;
		if (i <= j) {
			tmp_1 = arr[i][0];
			tmp_2 = arr[i][1];
			arr[i][0] = arr[j][0];
			arr[i][1] = arr[j][1];
			arr[j][0] = tmp_1;
			arr[j][1] = tmp_2;
			i++;
			j--;
		}
	};

	/* recursion */
	if (left < j)
		quickSort_2(arr, left, j);
	if (i < right)
		quickSort_2(arr, i, right);

}
/*For ChoiceFile*/
inline char* CorectName(char* file_name)
{
	for (int i = 0; i < 8; i++)
	{
		if (file_name[i] >= 'a' &&file_name[i] <= 'z')
			file_name[i] = file_name[i] - 32;
		else if (file_name[i] <= '/')
			file_name[i] = ' ';
		else if (file_name[i] > '9' &&file_name[i] < 'A')
			file_name[i] = ' ';
		else if (file_name[i] > 'Z')
			file_name[i] = ' ';
	}
	return file_name;
}
inline int InputFileNames(char* &file_name)
{

	cin.getline(file_name,8);

	file_name = CorectName(file_name);
	if ((char* &)file_name == 0)
		return -1;
	return 0;
}


int WriteMesage(HANDLE fdevice, FLASH_INFO f_info, FILE_INFO* files, int num_files)
{
	system("cls");
	int MASK = log2(num_files);
	unsigned _int32 size_one_cluster = f_info.bytes_per_sector*f_info.sector_per_cluster;
	unsigned _int32 size_stg = 0;
	unsigned _int32 sum_num_cluster = 0;

	int time_start = 0;
	int time_end = 0;

	FILE* mesage_file;
	char* stg; /*delete DONE*/
	unsigned _int32* num_cluster = new unsigned _int32[num_files]; /*delete DONE*/
	unsigned _int32* start_cluster = new unsigned _int32[num_files];/*delete DONE*/

	for (int i = 0; i < num_files; i++)
	{
		num_cluster[i] = ceil((double)files[i].FileSize / size_one_cluster);
		start_cluster[i] = GetFirstClusterOfFile(files[i]);
	}

	for (int i = 0; i < num_files; i++)
		sum_num_cluster += num_cluster[i];
	printf("Max size mesage - %i\n", (sum_num_cluster*MASK) / 8);

	bool All_ok = true;
	do {

		All_ok = true;
		mesage_file = FileOpenToRead(); /*close DONE*/
		while (mesage_file == 0)
		{
			printf("Incorect file name!\n ");
			mesage_file = FileOpenToRead();
		}
		int size_file = SizeFile(mesage_file);
		char * mesage = new char[size_file]; /*delete DONE*/
		fread(mesage, 1, size_file, mesage_file);

		size_stg = ((size_file * 8) / MASK + (bool)((size_file * 8) % MASK));
		printf("Generat steganogram..."); time_start = clock();
		stg = CreateInformArray(mesage, size_file, MASK);
		time_end = clock()-time_start;
		printf("DONE.\t\t\t%f\n", (float)time_end/CLOCKS_PER_SEC);
		delete[]mesage; /*mesage*/
		

		int* stg_type = new int[num_files];/*Delete*/
		for (int i = 0; i < num_files; i++)
			stg_type[i] = 0;
		for (int i = 0; i < size_stg; i++)
			stg_type[stg[i]]++;

		printf("\nnum stg type - num cluster file\n");
		for (int i = 0; i < num_files; i++)
		{
			if(stg_type[i]<=num_cluster[i])
			printf("%i) %i - %i\t - OK\n", i, stg_type[i], num_cluster[i]);
			else
			{
				fcloseall();
				All_ok = false;
				printf("%i) %i - %i\t - INVALID\n", i, stg_type[i], num_cluster[i]);
			}
		}
	} while (All_ok == false);

	int total_write_time_start = clock() + time_end;

	unsigned _int32** cluster_chain = new unsigned _int32*[num_files]; /*delete DONE*/

	printf("\nGet cluster chain..."); time_start = clock();
	for (int i = 0; i < num_files; i++)
		cluster_chain[i] = GetClusterChain(fdevice, f_info, start_cluster[i], num_cluster[i]);
	time_end = clock() - time_start;
	printf("DONE.\t\t\t%f\n", (float)time_end / CLOCKS_PER_SEC);

	printf("Load data file to bufer..."); time_start = clock();
	char** bufer_for_file_data; /*delete DONE*/
	bufer_for_file_data = LoadDataFilesToBuf(fdevice, f_info, cluster_chain, num_cluster, num_files);
	time_end = clock() - time_start;
	printf("DONE.\t\t\t%f\n", (float)time_end / CLOCKS_PER_SEC);

	printf("Transformation cluster chain..."); time_start = clock();
	cluster_chain = ChainTransformation(cluster_chain, start_cluster, num_cluster, num_files, stg, size_stg);
	time_end = clock() - time_start;
	printf("DONE.\t\t%f\n", (float)time_end / CLOCKS_PER_SEC);

	printf("Load data file to flash..."); time_start = clock();
	LoadDataFilesFromBuf(fdevice, f_info, cluster_chain, num_cluster, num_files, bufer_for_file_data);
	time_end = clock() - time_start;
	printf("DONE.\t\t\t%f\n", (float)time_end / CLOCKS_PER_SEC);

	printf("Load cluster chain to fat..."); time_start = clock();
	LoadClusterChainToFat(cluster_chain, fdevice, f_info, num_cluster, num_files);
	time_end = clock() - time_start;
	printf("DONE.\t\t%f\n", (float)time_end / CLOCKS_PER_SEC);

	printf("Reoaled files metadata..."); time_start = clock();
	ReloadFileInfo(fdevice, f_info, files, num_files, start_cluster);
	time_end = clock() - time_start;
	printf("DONE.\t\t\t%f\n", (float)time_end / CLOCKS_PER_SEC);

	delete[]stg;/*stg*/
	delete[]num_cluster;/*num_cluster*/
	delete[]start_cluster;/*start_cluster*/
	for (int i = 0; i < num_files; i++)
		delete[]cluster_chain[i]; /*cluster_chain[i]*/
	delete[]cluster_chain; /*cluster_chain*/
	delete bufer_for_file_data;/*bufer_for_file_data*/

	fcloseall();
	int total_write_time_end = clock() - total_write_time_start;
	printf("\nTotal time for write mesage - %f\n", (float)total_write_time_end / CLOCKS_PER_SEC);
	return 0;
}
/*For Write*/
inline _int32 MinimumSizeFile(FILE_INFO* files, int num_files)
{
	_int32 min = files[0].FileSize;
	for (int i = 1; i < num_files; i++)
		if (min > files[i].FileSize)
			min = files[i].FileSize;
	return min;
}
FILE* FileOpenToRead()
{
	char *str = new char[20];
	printf("\nName of mesage file, with type(20 symbols) : ");
	gets_s(str, 20);

	string folder = "mesage\\";
	folder += str;

	FILE* f;
	if (fopen_s(&f, folder.c_str(), "rb"))
	{
		delete[]str;
		return 0;
	}
	delete[]str;
	return f;
}
inline int SizeFile(FILE *f)
{
	fseek(f, 0, SEEK_END);
	int size = ftell(f);
	fseek(f, 0, SEEK_SET);
	return size;
}
char* CreateInformArray(char* mes, int size_file, int MASK)
{
	char* inform_array = new char[((size_file * 8) / MASK + (bool)((size_file * 8) % MASK))];
	for (int i = 0; i < ((size_file * 8) / MASK + (bool)((size_file * 8) % MASK)); i++)
		inform_array[i] = 0;

	for (int i = 0, num_stg = 0; i < (size_file * 8); i++)
	{
		if ((i%MASK) == 0 && i != 0)
			num_stg++;
		inform_array[num_stg] = (inform_array[num_stg] << 1) | ((mes[i / 8] >> (7 - (i % 8))) & 0x1);
	}
	return inform_array;
}
inline unsigned _int32 GetFirstClusterOfFile(FILE_INFO files)
{
	unsigned _int32 num_cluster = 0;
	num_cluster ^= files.FstClusHI;
	num_cluster = (num_cluster << 16) ^ files.FstClusLO;
	return num_cluster;
}
unsigned _int32* GetClusterChain(HANDLE hdevice, FLASH_INFO f_info, unsigned _int32 start_cluster, unsigned _int32 num_cluster)
{
	
	unsigned _int32 sectors_per_fat_file = (num_cluster / 128) + 2;
	unsigned _int32* table = new unsigned _int32[128];/*delete DONE*/
	unsigned _int32* chain = new unsigned _int32[num_cluster];/*delete*/
	unsigned _int32 chain_count = 0;
	DWORD Bytes_READ;

	for (unsigned _int32 i = 0; ; i++)
	{
		int fat_sector = (start_cluster / 128);
		SetFilePointer(hdevice, f_info.bytes_per_sector * (f_info.num_reserv_sector + fat_sector), NULL, FILE_BEGIN);
		ReadFile(hdevice, table, f_info.bytes_per_sector, &Bytes_READ, NULL);
		for (int j = 0; j < 128; j++)
		{
			chain[chain_count] = start_cluster;
			chain_count++;
			start_cluster = table[start_cluster % 128];
			if (start_cluster == 0x0FFFFFFF)
			{
				delete[]table;/*table*/
				return chain;
			}
			else if (start_cluster / 128 != (unsigned _int32)fat_sector)
				break;
			
		}
	}
}
char ** LoadDataFilesToBuf(HANDLE hdevice, FLASH_INFO f_info, unsigned _int32** cluster_chain, unsigned _int32* num_cluster, unsigned _int32 num_files)
{
	unsigned _int32 dir_sector = f_info.num_reserv_sector + (f_info.num_of_FAT * f_info.num_of_sector_per_FAT_32);
	unsigned _int32 size_one_cluster = f_info.bytes_per_sector*f_info.sector_per_cluster;

	char** bufer_for_data = new char*[num_files];
	for (int i = 0; i < num_files; i++)
		bufer_for_data[i] = new char[size_one_cluster*num_cluster[i]];

	DWORD Bytes_READ;
	for (int files = 0; files < num_files; files++)
	{
		Bytes_READ = 0;
		for (unsigned _int32 i = 0; i < num_cluster[files]; i++)
		{
			SetFilePointer(hdevice, (dir_sector*f_info.bytes_per_sector) + (size_one_cluster* (cluster_chain[files][i] - 2)), NULL, FILE_BEGIN);
			ReadFile(hdevice, &bufer_for_data[files][size_one_cluster*i], size_one_cluster, &Bytes_READ, NULL);
		}
		if (Bytes_READ = ! size_one_cluster)
		{
			delete[]bufer_for_data;
			return NULL;
		}
	}
	return bufer_for_data;
}
unsigned _int32** ChainTransformation(unsigned _int32** elder_cluster_chain, unsigned _int32* &start_cluster, unsigned _int32* num_cluster, int num_files, char* stegano_mesage, unsigned _int32 size_stegano_mesage)
{
	unsigned _int32 size_total_chain = 0;
	for (int i = 0; i < num_files; i++)
		size_total_chain += num_cluster[i];
	unsigned _int32* global_chain = new unsigned _int32[size_total_chain]; /*delete DONE*/

	for (int i = 0, global_position=0; i < num_files; i++)
		for (int j = 0; j < num_cluster[i]; j++, global_position++)
			global_chain[global_position] = elder_cluster_chain[i][j];

	quickSort(global_chain, 0, size_total_chain - 1);

	unsigned _int32** new_cluster_chain = new unsigned _int32*[num_files]; /*delete NON*/
	for (int i = 0; i < num_files; i++)
		new_cluster_chain[i] = new unsigned _int32[num_cluster[i]]; /*delete NON*/

	unsigned _int32* left_clusters_number = new unsigned _int32[num_files]; /*delete DONE*/
	for (int i = 0; i < num_files; i++)
		left_clusters_number[i] = num_cluster[i];

	int choice = 0;
	for (unsigned _int32 i = 0; i < size_total_chain; i++)
	{
		if (i < size_stegano_mesage)
		{
			new_cluster_chain[stegano_mesage[i]][num_cluster[stegano_mesage[i]]- left_clusters_number[stegano_mesage[i]]] = global_chain[i];
			left_clusters_number[stegano_mesage[i]]--;
		}
		else if (left_clusters_number[choice])
		{
			new_cluster_chain[choice][num_cluster[choice] - left_clusters_number[choice]] = global_chain[i];
			left_clusters_number[choice]--;
		}
		else 
		{
			choice++;
			i--;
		}
	}
	for (int i = 0; i < num_files; i++)
		start_cluster[i] = new_cluster_chain[i][0];

	for (int i = 0; i < num_files; i++)
		delete[]elder_cluster_chain[i];/*elder_cluster_chain[i]*/
	delete[]elder_cluster_chain;/*elder_cluster_chain[i]*/
	delete[]global_chain; /*global_chain*/
	delete[]left_clusters_number;/*left_clusters_number*/
	return new_cluster_chain;
}
unsigned _int32 LoadDataFilesFromBuf(HANDLE hdevice, FLASH_INFO f_info, unsigned _int32** cluster_chain, unsigned _int32* num_cluster, 	unsigned _int32 num_files, char** bufer_for_file_data)
{
	unsigned _int32 dir_sector = f_info.num_reserv_sector + (2 * f_info.num_of_sector_per_FAT_32);
	unsigned _int32 size_one_cluster = f_info.bytes_per_sector*f_info.sector_per_cluster;


	DWORD Bytes_Write;
	for (int files = 0; files < num_files; files++)
	{
		Bytes_Write = 0;
		for (unsigned _int32 i = 0; i < num_cluster[files]; i++)
		{
			SetFilePointer(hdevice, (dir_sector*f_info.bytes_per_sector) + (size_one_cluster* (cluster_chain[files][i] - 2)), NULL, FILE_BEGIN);
			WriteFile(hdevice, &bufer_for_file_data[files][size_one_cluster*i], size_one_cluster, &Bytes_Write, NULL);
		}
		if (Bytes_Write = !size_one_cluster)
			return NULL;
	}
	return Bytes_Write;
}
unsigned _int32 LoadClusterChainToFat(unsigned _int32** cluster_chain, HANDLE hdevice, FLASH_INFO f_info, unsigned _int32* num_cluster, unsigned _int32 num_files)
{
	unsigned _int32 size_one_cluster = f_info.bytes_per_sector*f_info.sector_per_cluster;
	unsigned _int32* table = new unsigned _int32[128]; /*delete DONE*/


	DWORD Bytes_READ;
	DWORD Bytes_WRITER;

	for (int files = 0; files < num_files; files++)
	{
		int fat_sector = (cluster_chain[files][0] / (f_info.bytes_per_sector / 4));
		Bytes_READ = 0;
		Bytes_WRITER = 0;
		SetFilePointer(hdevice, f_info.bytes_per_sector * (f_info.num_reserv_sector+ fat_sector), NULL, FILE_BEGIN);
		ReadFile(hdevice, table, f_info.bytes_per_sector, &Bytes_READ, NULL);
		for (unsigned _int32 i = 0; i < num_cluster[files]; i++)
		{
			if (cluster_chain[files][i] / (f_info.bytes_per_sector / 4) != fat_sector)
			{
				SetFilePointer(hdevice, (-1)*f_info.bytes_per_sector, NULL, FILE_CURRENT);
				WriteFile(hdevice, table, Bytes_READ, &Bytes_WRITER, NULL);

				fat_sector = (cluster_chain[files][i] / (f_info.bytes_per_sector / 4));

				SetFilePointer(hdevice, f_info.bytes_per_sector * (f_info.num_reserv_sector + fat_sector), NULL, FILE_BEGIN);
				ReadFile(hdevice, table, f_info.bytes_per_sector, &Bytes_READ, NULL);
			}
			if (i + 1 == num_cluster[files])
			{

				table[cluster_chain[files][i] % 128] = 0x0FFFFFFF;
				SetFilePointer(hdevice, (-1)*f_info.bytes_per_sector, NULL, FILE_CURRENT);
				WriteFile(hdevice, table, Bytes_READ, &Bytes_WRITER, NULL);
				break;
			}
			table[cluster_chain[files][i] % 128] = cluster_chain[files][i + 1];
		}
	}
	delete[]table;/*table*/
	return Bytes_WRITER;
}
int ReloadFileInfo(HANDLE fdevice, FLASH_INFO f_info, FILE_INFO* files, int num_files, unsigned _int32* start_cluster)
{
	unsigned _int32 dir_sector = f_info.num_reserv_sector + (2 * f_info.num_of_sector_per_FAT_32);
	char* buffer = new char[f_info.bytes_per_sector*f_info.sector_per_cluster]; /*delete DONE*/
	DWORD Bytes_READ;
	DWORD Bytes_WRITER;
	SetFilePointer(fdevice, dir_sector * f_info.bytes_per_sector, NULL, FILE_BEGIN);
	ReadFile(fdevice, buffer, f_info.bytes_per_sector*f_info.sector_per_cluster, &Bytes_READ, NULL);

	for (int x = 0; x < num_files; x++)
	{
		bool true_name = false;
		for (int i = 0; i < 16 * f_info.sector_per_cluster; i++)
		{
			for (short count = 0; count < 8; count++)
			{
				if (buffer[(i * 32) + count] != files[x].name[count] && buffer[(i * 32) + count] != 0x20)//buffer[(i * 32) + j]
				{
					true_name = false;
					break;
				}
				else true_name = true;
			}
			if (true_name)
			{
				(unsigned _int16&)buffer[(i * 32) + 20] = ((start_cluster[x] >> 16) & 0xFFFF);
				(unsigned _int16&)buffer[(i * 32) + 26] = start_cluster[x] & 0xFFFF;
				break;
			}

		}
	}
	SetFilePointer(fdevice, (-1)*f_info.bytes_per_sector*f_info.sector_per_cluster, NULL, FILE_CURRENT);
	WriteFile(fdevice, buffer, Bytes_READ, &Bytes_WRITER, NULL);
	delete[]buffer;/*buffer*/
	return Bytes_READ;
}

int ReadMesage(HANDLE fdevice, FLASH_INFO f_info, FILE_INFO* files, int num_files)
{
	system("cls");
	int MASK = log2(num_files);
	unsigned _int32 size_one_cluster = f_info.bytes_per_sector*f_info.sector_per_cluster;
	char* stg; /*delete*/

	int time_start = 0;
	int time_end = 0;
	int total_write_time_start = clock();

	unsigned _int32* num_cluster = new unsigned _int32[num_files]; /*delete */
	unsigned _int32* start_cluster = new unsigned _int32[num_files];/*delete */
	for (int i = 0; i < num_files; i++)
	{
		num_cluster[i] = ceil((double)files[i].FileSize / size_one_cluster);
		start_cluster[i] = GetFirstClusterOfFile(files[i]);
	}
	unsigned _int32 all_num_cluster = 0;
	for (int i = 0; i < num_files; i++)
		all_num_cluster += num_cluster[i];
	unsigned _int32** cluster_chain = new unsigned _int32*[num_files]; /*delete*/

	printf("Get cluster chain..."); time_start = clock();
	for (int i = 0; i < num_files; i++)
		cluster_chain[i] = GetClusterChain(fdevice, f_info, start_cluster[i], num_cluster[i]);
	time_end = clock() - time_start;
	printf("DONE.\t\t\t%f\n", (float)time_end / CLOCKS_PER_SEC);

	printf("Reading steganoblocks..."); time_start = clock();
	stg = ReadSteaganoFromClusterChain(cluster_chain, num_cluster, num_files, all_num_cluster);
	time_end = clock() - time_start;
	printf("DONE.\t\t\t%f\n", (float)time_end / CLOCKS_PER_SEC);

	printf("Conglutination message..."); time_start = clock();
	char* mesage = CreateMesageFromStg(stg, all_num_cluster, MASK);
	time_end = clock() - time_start;
	printf("DONE.\t\t\t%f\n", (float)time_end / CLOCKS_PER_SEC);

	int total_write_time_end = clock() - total_write_time_start;

	unsigned _int32 mesage_size = ((all_num_cluster*MASK) / 8) + ((bool)(all_num_cluster*MASK) % 8);
	FILE* mesage_file = FileOpenToWrite(); /*close */
		fwrite(mesage, 1, mesage_size, mesage_file);

	printf("\nTotal time for read mesage - %f\n", (float)total_write_time_end / CLOCKS_PER_SEC);
		fcloseall();
		return 0;
}
FILE* FileOpenToWrite()
{
	char *str = new char[20];
	printf("\nName of mesage file, with type(20 symbols) : ");
	gets_s(str, 20);

	string folder = "mesage\\";
	folder += str;

	FILE* f;
	if (fopen_s(&f, folder.c_str(), "wb"))
	{
		delete[]str;
		return 0;
	}
	delete[]str;
	return f;
}
char* ReadSteaganoFromClusterChain(unsigned _int32** cluster_chain, unsigned _int32* num_cluster, unsigned _int32 num_files, unsigned _int32 size_stegano_mesage)
{
	unsigned _int32 size_total_chain = 0;
	for (int i = 0; i < num_files; i++)
		size_total_chain += num_cluster[i];
	unsigned _int32** global_chain = new unsigned _int32*[size_total_chain]; /*delete*/
	for (int i = 0; i < size_total_chain; i++)
		global_chain[i] = new  unsigned _int32[2];/*delete*/

	for (int i = 0, global_position = 0; i < num_files; i++)
		for (int j = 0; j < num_cluster[i]; j++, global_position++)
		{
			global_chain[global_position][0] = cluster_chain[i][j];
			global_chain[global_position][1] = i;
		}


	quickSort_2(global_chain, 0, size_total_chain - 1);

	char* stegano_mesage = new char[size_stegano_mesage];
	for (unsigned _int32 i = 0; i < size_stegano_mesage; i++)
		stegano_mesage[i] = global_chain[i][1];

	return stegano_mesage;
}
char* CreateMesageFromStg(char* stg, int size_stg, int MASK)
{
	char* mesage = new char[((size_stg*MASK) / 8) + ((bool)(size_stg*MASK) % 8)];
	for (int i = 0; i < ((size_stg*MASK) / 8) + ((bool)(size_stg*MASK) % 8); i++)
		mesage[i] = 0;

	for (int i = 0; i < (size_stg*MASK); i++)
		mesage[i / 8] = (mesage[i / 8] << 1) | ((stg[i / MASK] >> ((MASK - 1) - (i%MASK))) & 0x1);
	return mesage;
}