#include "prech.h"
#include <stdio.h>  /* defines FILENAME_MAX */


#include "Compressor.h"
int main() {
	
	//std::cout << MC::text();
	//std::cin.get();

	//char buff[FILENAME_MAX];
	//GetCurrentDir(buff, FILENAME_MAX);
	//std::string current_working_dir(buff);
	//std::cout << current_working_dir.c_str() << std::endl;

	//FILE * pFile;
	//long lSize;
	//char * buffer;
	//size_t result;
	//
	//pFile = fopen("manbody.obj", "r");
	//if (pFile == NULL) { fputs("File error", stderr); 
	//printf("\nOh dear, something went wrong with read()! %s\n", strerror(errno)); 
	//exit(1); }

	//// obtain file size:
	//fseek(pFile, 0, SEEK_END);
	//lSize = ftell(pFile);
	//rewind(pFile);

	//// allocate memory to contain the whole file:
	//buffer = (char*)malloc(sizeof(char)*lSize);
	//if (buffer == NULL) { fputs("Memory error", stderr); exit(2); }
	//memset(buffer, 0, lSize);
	//// copy the file into the buffer:
	//result = fread(buffer, 1, lSize, pFile);
	//if (result !=lSize) { fputs("Reading error", stderr); exit(3); }

	///* the whole file is now loaded in the memory buffer. */
	///*for (int i = 0; i < result; i++) {
	//	printf("%d %c ", buffer[i], buffer[i]);

	//}*/
	//fclose(pFile);
	//pFile = fopen("temp.bin", "wb");
	//if (pFile == NULL) {
	//	fputs("File error", stderr);
	//	printf("\nOh dear, something went wrong with read()! %s\n", strerror(errno));
	//	exit(1);
	//}
	//byte* bytes = new byte[result];

	//fwrite(bytes, sizeof(char), result, pFile);
	//rewind(pFile);
	//// terminate
	//fclose(pFile);
	//free(buffer);

	Compressor prueba;
	prueba.CompressModel("fire2.obj");
	//prueba.CompressModel("manbody.obj");
	std::cout << "se fini" << std::endl;
	//std::cin.get();
	return 0;
}