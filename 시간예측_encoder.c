#include<stdio.h>
#include<math.h>
#include <stdlib.h>
#include <time.h>
#include<string.h>
//#include "encoder_intra.h"
//#include "file_read_or_write.h"
# define org_row  720
# define org_col  480
# define sizebyte 1
//# define in_file_bf "lena.img"
# define in_file_bf "football_bf.y"
# define in_file_af "football_af.y"
# define reconstruct_file_bf "0_before_encoding.y"
# define label_file "1_label"
# define error_file "2_error"
# define MV_x_file "3_motion_vector_x.y"
# define MV_y_file "4_motion_vector_y.y"
# define Quantization_file "5_몫.y"
# define current_encoding_file "0_current_encodiong.y"
# define prediction_defore "0_prediction.y"
# define N_intra 4// 공간예측 block
# define N_tempo 16// 시간예측 block
# define use_psnr 1// (0 : psnr 측정 x   , 1 : psnr 측정 o ) 
# define FALSE 0
# define TRUE 1
# define sample 1.75 // 양자화 계수
# define pre_N 9 // 예측 방향 (4 or 9)
//====option================
# define SR 4// search range => 4x4, 8x8, 16x16

//파일 읽기
unsigned char* readFile(char* s, int size_row, int size_col) {
	//FILE* input_img = fopen(in_file, "rb");
	FILE* input_img;
	unsigned char* org_img = NULL;
	fopen_s(&input_img, s, "rb");

	org_img = (unsigned char*)malloc(sizeof(unsigned char)*(size_row*size_col));
	if (input_img == NULL) {
		puts("input 파일 오픈 실패 ");
		return NULL;
	}
	else {
		fseek(input_img, 0, SEEK_SET);
		fread((void*)org_img, sizebyte, size_row * size_col, input_img);
	}
	return org_img;
	fclose(input_img);
	free(org_img);
}

//파일 읽기
int* ReadFile_int(char* s, int size_row, int size_col) {
	//FILE* input_img = fopen(in_file, "rb");
	FILE* input;
	int* org = NULL;

	fopen_s(&input, s, "rb");
	org = (int*)malloc(sizeof(int)*(org_row*org_col));
	if (input == NULL) {
		puts("input 파일 오픈 실패 ");
		return NULL;
	}
	else {
		fseek(input, 0, SEEK_SET);
		fread((void*)org, sizeof(int), org_row*org_col, input);
	}
	fclose(input);
	return org;
	free(org);
}

//unsigned char 형 파일 쓰기
unsigned char* WriteFile_U(unsigned char* out_img, char* s, int size_row, int size_col) {
	//FILE* output_img = fopen(out_file, "wb");
	FILE* output_img;
	fopen_s(&output_img, s, "wb");
	if (output_img == NULL) {
		puts("output 파일 오픈 실패");
		return NULL;
	}
	else {
		fseek(output_img, 0, SEEK_SET);
		fwrite((void*)out_img, sizebyte, size_row*size_col, output_img);
	}
	fclose(output_img);
	return out_img;

}

// int 형 파일 쓰기
int* WriteFile_I(int* out_img, char* s, int size_row, int size_col) {
	FILE* output_img;
	fopen_s(&output_img, s, "wb");
	if (output_img == NULL) {
		puts("output 파일 오픈 실패");
		return NULL;
	}
	else {
		fseek(output_img, 0, SEEK_SET);
		fwrite((void*)out_img, sizeof(int), size_row*size_col, output_img);
	}
	fclose(output_img);
	return out_img;
}

//MSE & PSNR **입력은 uchar 형**
double MSE_f(unsigned char* sp_img, char* s)
{
	double MSE;
	double PSNR;
	double sum = 0.0;
	int i, j;
	//FILE* in2 = fopen("lena.img", "rb");
	unsigned char* org_img2 = NULL;
	FILE* in2;
	fopen_s(&in2, s, "rb");
	org_img2 = (unsigned char*)malloc(sizeof(unsigned char)*(org_row*org_col));
	fseek(in2, 0, SEEK_SET);
	fread((void*)org_img2, sizebyte, org_row*org_col, in2);
	for (i = 0; i < org_col; i++)
	{
		for (j = 0; j< org_row; j++) {
			double temp = (double)(*(org_img2 + i*org_row + j)) - (double)(*(sp_img + i*org_row + j));
			sum += pow(temp, 2); //sum += temp*temp
		}
	}
	MSE = (long double)sum / ((double)org_row*(double)org_col);
	PSNR = 10 * log10(255.0*255.0 / MSE);
	fseek(in2, 0, SEEK_SET);
	printf("\n\nMSE = %f\n\n", MSE);
	printf("\n\nPSNR = %f\n\n", PSNR);
	fclose(in2);
	free(org_img2);
	return MSE;
}

// error sampling, 양자화 계수를 나눠줌 =================수정함====decoder 도 같이 바꿔줄 것=============
int* sampling_error(int* error, int type, int size_row, int size_col) {
	int n;
	int* error_d = NULL;
	error_d = (int*)malloc(sizeof(int)*(size_row*size_col));
	//type 이 0이면 error 를 양자화만 함
	if (type == 0) {
		for (n = 0; n < size_row*size_col; n++) {
			if ((*(error + n)) > 0) {
				*(error_d + n) = (int)((double)(*(error + n)) / sample + 0.5);
			}
			else if ((*(error + n)) < 0) {
				*(error_d + n) = (int)((double)(*(error + n)) / sample - 0.5);
			}
			else
				*(error_d + n) = 0;

		}
		return error_d;
	}

	//type = 1 양자화된 에러를 다시 복구함 
	else if (type == 1) {
		for (n = 0; n < size_row*size_col; n++) {
			*(error_d + n) = (int)((double)(*(error + n))*sample + 0.5);

			if ((*(error + n)) > 0) {
				*(error_d + n) = (int)((double)(*(error + n))*sample + 0.5);
			}
			else if ((*(error + n)) < 0) {
				*(error_d + n) = (int)((double)(*(error + n))*sample - 0.5);
			}
			else
				*(error_d + n) = 0;
			*(error_d + n) = (*(error + n))*sample;
		}
		return error_d;
	}
	return NULL;
}


//공간예측=============================================================================================================

//블록의 이웃한 픽셀 축력하기
unsigned char* neighbor_pixels(unsigned char* org_img, int i, int j) { // i = 행, j = 열
	int u, v;
	int w;
	unsigned char* neighbor_pix = NULL;
	neighbor_pix = (unsigned char*)malloc(sizeof(unsigned char)*(N_intra + pre_N)); //8 or 13
	if (org_img == NULL) {
		for (w = 0; w < pre_N; w++) {
			*(neighbor_pix + w) = 128;
		}
	}

	else {
		//A,B,C,D
		for (u = 0; u < N_intra; u++) {
			if ((j - 1) >= 0)
				*(neighbor_pix + u) = *(org_img + (j - 1)*org_row + i + u);
			else
				*(neighbor_pix + u) = 128;
		}

		//I,J,K,L
		for (v = 0; v < N_intra; v++) {
			if ((i - 1) >= 0)
				*(neighbor_pix + N_intra + v) = *(org_img + (j + v)*org_row + i - 1);
			else
				*(neighbor_pix + N_intra + v) = 128;
		}
		if (pre_N == 9) {
			//E,F,G,H
			if (i + N_intra + 3 <= org_row - 1) {
				for (u = 0; u < N_intra; u++) {
					if ((j - 1) >= 0)
						*(neighbor_pix + 2 * N_intra + u) = *(org_img + (j - 1)*org_row + i + u + N_intra);
					else
						*(neighbor_pix + 2 * N_intra + u) = 128;
				}
			}
			else {
				for (u = 0; u < N_intra; u++) {
					if ((j - 1) >= 0)
						*(neighbor_pix + 2 * N_intra + u) = *(org_img + (j - 1)*org_row + i + 3);
					else
						*(neighbor_pix + 2 * N_intra + u) = 128;
				}
			}
			//Q
			if ((j - 1) >= 0 && (i - 1) >= 0)
				*(neighbor_pix + 3 * N_intra) = *(org_img + (j - 1)*org_row + i - 1);
			else
				*(neighbor_pix + 3 * N_intra) = 128;
		}
	}
	return  neighbor_pix;
	free(neighbor_pix);
}

//예측 블록 출력  ==========mode 3,4 고쳐서 decoding 에 복사하기
unsigned char* pre_block_intra(unsigned char* neighbor_pix, int type, int ii, int jj) {
	unsigned char* block = NULL;

	int i, u, v;
	unsigned char m;
	//double sum = 0.0;
	int sum = 0;

	int A = *(neighbor_pix);
	int B = *(neighbor_pix + 1);
	int C = *(neighbor_pix + 2);
	int D = *(neighbor_pix + 3);
	int I = *(neighbor_pix + 4);
	int J = *(neighbor_pix + 5);
	int K = *(neighbor_pix + 6);
	int L = *(neighbor_pix + 7);
	int E, F, G, H, Q;
	if (pre_N == 9) {
		E = *(neighbor_pix + 8);
		F = *(neighbor_pix + 9);
		G = *(neighbor_pix + 10);
		H = *(neighbor_pix + 11);
		Q = *(neighbor_pix + 12);
	}

	block = (unsigned char*)malloc(sizeof(unsigned char)*(N_intra*N_intra));

	if (type == 0) {
		for (v = 0; v < N_intra; v++) {
			for (u = 0; u < N_intra; u++) {
				*(block + v*N_intra + u) = *(neighbor_pix + u);
			}
		}
	}
	else if (type == 1) {
		for (v = 0; v < N_intra; v++) {
			for (u = 0; u < N_intra; u++) {
				*(block + v*N_intra + u) = *(neighbor_pix + v + N_intra);
			}
		}
	}

	else if (type == 2) {
		sum = 0;
		if (ii - 1 >= 0 && jj - 1 >= 0) {
			for (i = 0; i < 2 * N_intra; i++) {

				sum += *(neighbor_pix + i);
			}
			m = (sum + 4) >> 3;
			
		}
		else if (ii - 1 < 0 && jj - 1 >= 0) { // 세로 막대 = 128
			for (i = 0; i < N_intra; i++) {
				sum += *(neighbor_pix + i);
			}
			m = (unsigned char)(sum / ((double)N_intra) + 0.5);
		}
		else if (ii - 1 >= 0 && jj - 1 < 0) { // 가로 막대 = 128
			for (i = 0; i < N_intra; i++) {
				sum += *(neighbor_pix + N_intra + i);
			}
			m = (unsigned char)(sum / ((double)N_intra) + 0.5);
		}
		else
			m = 128;
			
		for (v = 0; v < N_intra; v++) {
			for (u = 0; u < N_intra; u++) {
				*(block + v*N_intra + u) = m;
			}
		}
	}

	else if (type == 3) {
		if (pre_N == 4) {
			for (i = 0; i < N_intra*N_intra; i++) {
				if (i == 0 || i == 5 || i == 10 || i == 15)
					*(block + i) = (A + I + 1) >> 1;
				else if (i == 1 || i == 6 || i == 11)
					*(block + i) = (A + B + 1) >> 1;
				else if (i == 2 || i == 7)
					*(block + i) = (B + C + 1) >> 1;
				else if (i == 3)
					*(block + i) = (C + D + 1) >> 1;
				else if (i == 4 || i == 9 || i == 14)
					*(block + i) = (I + J + 1) >> 1;
				else if (i == 8 || i == 13)
					*(block + i) = (J + K + 1) >> 1;
				else if (i == 12)
					*(block + i) = (K + L + 1) >> 1;
			}
		}
		else if (pre_N == 9) {
			for (i = 0; i < N_intra*N_intra; i++) {
				if (i == 0)
					*(block + i) = (A + 2 * B + C + 2) >> 2;
				else if (i == 1 || i == 4)
					*(block + i) = (B + 2 * C + D + 2) >> 2;
				else if (i == 2 || i == 5 || i == 8)
					*(block + i) = (C + 2 * D + E + 2) >> 2;
				else if (i == 3 || i == 6 || i == 9 || i == 12)
					*(block + i) = (D + 2 * E + F + 2) >> 2;
				else if (i == 7 || i == 10 || i == 13)
					*(block + i) = (E + 2 * F + G + 2) >> 2;
				else if (i == 11 || i == 14)
					*(block + i) = (F + 2 * G + H + 2) >> 2;
				else if (i == 15)
					*(block + i) = (G + 3 * H + 2) >> 2;
			}
		}
	}
	if (pre_N == 9) {
		if (type == 4) {
			for (i = 0; i < N_intra*N_intra; i++) {
				if (i == 0 || i == 5 || i == 10 || i == 15)
					*(block + i) = (A + 2 * Q + I + 2) >> 2;
				else if (i == 1 || i == 6 || i == 11)
					*(block + i) = (Q + 2 * A + B + 2) >> 2;
				else if (i == 2 || i == 7)
					*(block + i) = (A + 2 * B + C + 2) >> 2;
				else if (i == 3)
					*(block + i) = (B + 2 * C + D + 2) >> 2;
				else if (i == 4 || i == 9 || i == 14)
					*(block + i) = (Q + 2 * I + J + 2) >> 2;
				else if (i == 8 || i == 13)
					*(block + i) = (I + 2 * J + K + 2) >> 2;
				else if (i == 12)
					*(block + i) = (J + 2 * K + L + 2) >> 2;
			}
		}
		else if (type == 5) {
			for (i = 0; i < N_intra*N_intra; i++) {
				if (i == 0)
					*(block + i) = (A + B + 1) >> 1;
				else if (i == 1 || i == 8)
					*(block + i) = (B + C + 1) >> 1;
				else if (i == 2 || i == 9)
					*(block + i) = (C + D + 1) >> 1;
				else if (i == 3 || i == 10)
					*(block + i) = (D + E + 1) >> 1;
				else if (i == 11)
					*(block + i) = (E + F + 1) >> 1;
				else if (i == 4)
					*(block + i) = (A + 2 * B + C + 2) >> 2;
				else if (i == 5 || i == 12)
					*(block + i) = (B + 2 * C + D + 2) >> 2;
				else if (i == 6 || i == 13)
					*(block + i) = (C + 2 * D + E + 2) >> 2;
				else if (i == 7 || i == 14)
					*(block + i) = (D + 2 * E + F + 2) >> 2;
				else if (i == 15)
					*(block + i) = (E + 2 * F + G + 2) >> 2;
			}
		}
		else if (type == 6) {
			for (i = 0; i < N_intra*N_intra; i++) {
				if (i == 0 || i == 6)
					*(block + i) = (Q + I + 1) >> 1;
				else if (i == 1 || i == 7)
					*(block + i) = (I + 2 * Q + A + 2) >> 2;
				else if (i == 2)
					*(block + i) = (Q + 2 * A + B + 2) >> 2;
				else if (i == 3)
					*(block + i) = (A + 2 * B + C + 2) >> 2;
				else if (i == 4 || i == 10)
					*(block + i) = (I + J + 1) >> 1;
				else if (i == 5 || i == 11)
					*(block + i) = (Q + 2 * I + J + 2) >> 2;
				else if (i == 8 || i == 14)
					*(block + i) = (J + K + 1) >> 1;
				else if (i == 9 || i == 15)
					*(block + i) = (I + 2 * J + K + 2) >> 2;
				else if (i == 12)
					*(block + i) = (K + L + 1) >> 1;
				else if (i == 13)
					*(block + i) = (J + 2 * K + L + 2) >> 2;
			}
		}
		else if (type == 7) {
			for (i = 0; i < N_intra*N_intra; i++) {
				if (i == 0 || i == 9)
					*(block + i) = (Q + A + 1) >> 1;
				else if (i == 1 || i == 10)
					*(block + i) = (A + B + 1) >> 1;
				else if (i == 2 || i == 11)
					*(block + i) = (B + C + 1) >> 1;
				else if (i == 3)
					*(block + i) = (C + D + 1) >> 1;
				else if (i == 4 || i == 13)
					*(block + i) = (I + 2 * Q + A + 2) >> 2; //****************************
				else if (i == 5 || i == 14)
					*(block + i) = (Q + 2 * A + B + 2) >> 2;
				else if (i == 6 || i == 15)
					*(block + i) = (A + 2 * B + C + 2) >> 2;
				else if (i == 7)
					*(block + i) = (B + 2 * C + D + 2) >> 2;
				else if (i == 8)
					*(block + i) = (Q + 2 * I + J + 2) >> 2;
				else if (i == 12)
					*(block + i) = (I + 2 * J + K + 2) >> 2;
			}
		}
		else if (type == 8) {
			for (i = 0; i < N_intra*N_intra; i++) {
				if (i == 0)
					*(block + i) = (I + J + 1) >> 1;
				else if (i == 1)
					*(block + i) = (I + 2 * J + K + 2) >> 2;
				else if (i == 2 || i == 4)
					*(block + i) = (J + K + 1) >> 1;
				else if (i == 3 || i == 5)
					*(block + i) = (J + 2 * K + L + 2) >> 2;
				else if (i == 6 || i == 8)
					*(block + i) = (K + L + 1) >> 1;
				else if (i == 7 || i == 9)
					*(block + i) = (K + 3 * L + 2) >> 2;
				else if (i >= 10)
					*(block + i) = L;
			}
		}
	}
	return block;
	free(block);
}

//한 block label 을 결정
int* label_finder(unsigned char* org_img, int i, int j) { //i = 열, j = 행
	int u, v;
	double sum = 0.0;
	int W;
	int min;
	int label;
	unsigned char* block_img = NULL;
	unsigned char* neighbor_pix = NULL;
	unsigned char* block_pre = NULL;
	int* Energy = NULL;
	int* error = NULL;
	int* out = NULL; // 출력 값 (label + error)



	block_img = (unsigned char*)malloc(sizeof(unsigned char)*(N_intra*N_intra));
	neighbor_pix = (unsigned char*)malloc(sizeof(unsigned char)*(N_intra + pre_N));
	block_pre = (unsigned char*)malloc(sizeof(unsigned char)*(N_intra*N_intra));
	Energy = (int*)malloc(sizeof(int)*(pre_N));
	error = (int*)malloc(sizeof(int)*(N_intra*N_intra));
	out = (int*)malloc(sizeof(int)*(1 + N_intra*N_intra));

	for (v = 0; v < N_intra; v++) {
		for (u = 0; u < N_intra; u++) {
			*(block_img + v*N_intra + u) = *(org_img + (i + v)*org_row + j + u);
		}
	}

	neighbor_pix = neighbor_pixels(org_img, j, i);

	for (W = 0; W < pre_N; W++) {
		*(Energy + W) = 0;
		block_pre = pre_block_intra(neighbor_pix, W, i, j);
		for (v = 0; v < N_intra; v++) {
			for (u = 0; u < N_intra; u++) {
				*(Energy + W) += (*(block_img + v*N_intra + u) - *(block_pre + v*N_intra + u))* (*(block_img + v*N_intra + u) - *(block_pre + v*N_intra + u));
			}
		}
	}

	label = 0;
	min = *(Energy + 0);
	for (W = 0; W < pre_N; W++) {
		if (min  > *(Energy + W)) {
			min = *(Energy + W);
			label = W;
		}
	}

	*(out) = label;
	block_pre = pre_block_intra(neighbor_pix, label, i, j);
	for (v = 0; v < N_intra; v++) {
		for (u = 0; u < N_intra; u++) {
			*(error + v*N_intra + u) = *(block_img + v*N_intra + u) - *(block_pre + v*N_intra + u);

		}
	}


	for (W = 0; W < N_intra*N_intra; W++) {
		*(out + W + 1) = *(error + W);
	}


	return out;
	free(block_img);
	free(block_pre);
	free(neighbor_pix);
	free(Energy);
	free(error);
	free(out);
}

//test 용====================
void sort_Label(unsigned char* Label_arr) {

	int* label_N = NULL;
	int i, j, w;

	label_N = (int*)malloc(sizeof(int)*(pre_N));
	for (w = 0; w < pre_N; w++) {
		*(label_N + w) = 0;
	}

	for (i = 0; i < org_col / N_intra; i++) {
		for (j = 0; j < org_row / N_intra; j++) {
			for (w = 0; w < pre_N; w++) {
				if (*(Label_arr + i*(org_row / N_intra) + j) == w) {
					*(label_N + w) += 1;
				}
			}
		}
	}
	printf("\n");
	for (w = 0; w < pre_N; w++) {
		printf("number of %d label :  %d\n", w, *(label_N + w));
	}

	free(label_N);
}

//test 용====================
void sort_Error(int* Error) {

	int* error_N = NULL;
	int i, j, w;

	error_N = (int*)malloc(sizeof(int)*(256 * 2));
	for (w = 0; w < 256 * 2; w++) {
		*(error_N + w) = 0;
	}

	for (i = 0; i < org_col; i++) {
		for (j = 0; j < org_row; j++) {
			for (w = 0; w < 2 * 256; w++) {
				if (*(Error + i*(org_row / N_intra) + j) == w - 255) {
					*(error_N + w) += 1;
				}
			}
		}
	}
	printf("\n");
	for (w = -15 + 255; w < 15 + 255; w++) {
		printf("number of %d error :  %d\n", w - 255, *(error_N + w));
	}

	free(error_N);
}

//(N/2)*(N/2) 차원의 각 block label 이 저장된 배열을 만드는 함수 ================실행 안됨
unsigned char* encoding_intra(unsigned char* org_img) { // 나중에 int 형을 unsigned char 형으로 바꿔보기
	int i, j, w;
	int u, v;
	int n = 0;//===========================
	int label;
	int* error = NULL;
	int* error_sampling = NULL;
	int* buff = NULL;
	int* error_array = NULL;
	unsigned char* recontruct_img = NULL;
	unsigned char* label_array = NULL;
	unsigned char* block_pre = NULL;
	unsigned char* neighbor_pix = NULL;

	recontruct_img = (unsigned char*)malloc(sizeof(unsigned char)*(org_col*org_row));
	label_array = (unsigned char*)malloc(sizeof(unsigned char)*(org_col*org_row / (N_intra*N_intra)));
	error = (int*)malloc(sizeof(int)*(N_intra*N_intra));
	error_sampling = (int*)malloc(sizeof(int)*(N_intra*N_intra));
	error_array = (int*)malloc(sizeof(int)*(org_col*org_row));
	block_pre = (unsigned char*)malloc(sizeof(unsigned char)*(N_intra*N_intra));
	buff = (int*)malloc(sizeof(int)*(N_intra*N_intra + 1));
	neighbor_pix = (unsigned char*)malloc(sizeof(unsigned char)*(N_intra + pre_N));

	for (i = 0; i < org_col; i++) {
		for (j = 0; j < org_row; j++) {
			*(recontruct_img + i*org_row + j) = *(org_img + i*org_row + j);
		}
	}

	for (i = 0; i < org_col; i += N_intra) {
		for (j = 0; j < org_row; j += N_intra) {
			//label, error 각각 저장
			buff = label_finder(recontruct_img, i, j);
			label = *(buff);
			//==============================================================
			if (label == 2) {
				n++;
			}
			//=============================================================

			for (w = 0; w < N_intra*N_intra; w++) {
				*(error + w) = *(buff + w + 1);
			}
			error_sampling = sampling_error(error, 0, N_intra, N_intra);//error 를 양자화 
			error = sampling_error(error_sampling, 1, N_intra, N_intra);//양자화된 error 복구

			neighbor_pix = neighbor_pixels(recontruct_img, j, i);//두번째 인수 : 행, 세번째 인수 : 열
			block_pre = pre_block_intra(neighbor_pix, label, i, j);
			//원본 영상의 해당 블록 대체, out 마지막에 label 을 저장
			for (v = 0; v < N_intra; v++) {
				for (u = 0; u < N_intra; u++) {
					int temp = *(block_pre + N_intra*v + u) + *(error + N_intra*v + u);
					//org_img update
					if (temp < 0)
						*(recontruct_img + (i + v)*org_row + (j + u)) = 0;
					else if (temp > 255)
						*(recontruct_img + (i + v)*org_row + (j + u)) = 255;
					else
						*(recontruct_img + (i + v)*org_row + (j + u)) = temp;

					//out 에 양자화된 error  저장
					*(error_array + (i + v)*org_row + (j + u)) = *(error_sampling + N_intra*v + u);
				}
			}
			*(label_array + (i / N_intra)*(org_row / N_intra) + j / N_intra) = (unsigned char)label;

		}
	}
	WriteFile_U(recontruct_img, reconstruct_file_bf, org_row, org_col);
	WriteFile_U(label_array, label_file, org_row / N_intra, org_col / N_intra);
	WriteFile_I(error_array, error_file, org_row, org_col);
	printf("\n2 = %d\n", n);//===================================
	sort_Label(label_array);

	free(error);
	free(error_array);
	free(error_sampling);
	free(buff);
	free(neighbor_pix);
	free(block_pre);
	free(label_array);
	return recontruct_img;
	free(recontruct_img);

}
//==============================================================================================================

//(미완)motion vector 정보를 받으면, 예측 블록 출력
unsigned char* pre_block_tempo(unsigned char* before_dec_img, int i, int j, int mv_x, int mv_y) {
	unsigned char* block_pre = NULL;
	int u, v;
	block_pre = (unsigned char*)malloc(sizeof(unsigned char)*(N_tempo*N_tempo));

	for (u = 0; u < N_tempo; u++) {
		for (v = 0; v < N_tempo; v++) {
			*(block_pre + u*N_tempo + v) = *(before_dec_img + ((i + mv_y) + u)*org_row + ((j + mv_x) + v));
		}
	}

	return block_pre;
	free(block_pre);
}

//(미완)블록의 현재 위치를 주면, search range를 구하고, 그안에 가장 적절한 블록을 향하는 motion vector 구하기==============가장가리를 넘어갈때 고려하는거 추가하기
int* mv_finder(unsigned char* current_img, unsigned char* before_dec_img, int i, int j) {
	int u, v;
	int n, m;
	int sum;
	int min;
	int left_row = 0;
	int right_row = 0;
	int top_cul = 0;
	int bottom_cul = 0;
	//int part_sum;
	int* motion_vector = NULL;
	int* energy = NULL;
	unsigned char* block_pre = NULL;
	unsigned char* block = NULL;

	block_pre = (unsigned char*)malloc(sizeof(unsigned char)*(N_tempo*N_tempo));
	block = (unsigned char*)malloc(sizeof(unsigned char)*(N_tempo*N_tempo));
	motion_vector = (int*)malloc(sizeof(int) * 2);



	for (n = 0; n < N_tempo; n++) {
		for (m = 0; m < N_tempo; m++) {
			*(block + n*N_tempo + m) = *(current_img + (i + n)*org_row + j + m);
		}
	}

	if (j - SR < 0)
		left_row = (-1)*j;
	else
		left_row = (-1)*SR;
	if ((j + N_tempo - 1) + SR >= org_row)
		right_row = (org_row - 1) - (j + N_tempo - 1);
	else
		right_row = SR;
	if (i - SR < 0)
		top_cul = (-1)*i;
	else
		top_cul = (-1)*SR;
	if ((i + N_tempo - 1) + SR >= org_col)
		bottom_cul = (org_col - 1) - (i + N_tempo - 1);
	else
		bottom_cul = SR;
	//printf("left row :%d\n", left_row);//test 용===============
	//printf("right row :%d\n", right_row);//test 용===============
	//printf("top cul :%d\n", top_cul);//test 용===============
	//printf("bottom cul :%d\n", bottom_cul);//test 용===============

	energy = (int*)malloc(sizeof(int) * (bottom_cul - top_cul + 1)*(right_row - left_row + 1));
	//search_range 안에 하나의 16 by 16 블록을 만들어(만약 영상의 가장자리를 넘어간다면, -1 저장) 복원영상과의 energy 저장
	for (u = top_cul; u <= bottom_cul; u++) {
		for (v = left_row; v <= right_row; v++) {
			*(energy + (u - top_cul)*(right_row - left_row + 1) + (v - left_row)) = 0;

			//printf("%d\n",(u - top_cul)*(right_row - left_row + 1) + (v-left_row)) ;//test 용=====================

			block_pre = pre_block_tempo(before_dec_img, i, j, v, u); //v = mv_x, u = mv_y
			sum = 0;
			for (n = 0; n < N_tempo; n++) {
				for (m = 0; m < N_tempo; m++) {
					*(energy + (u - top_cul)*(right_row - left_row + 1) + (v - left_row)) += (*(block + n*N_tempo + m) - *(block_pre + n*N_tempo + m))*(*(block + n*N_tempo + m) - *(block_pre + n*N_tempo + m));
				}
			}
		}
	}

	min = *(energy);
	*(motion_vector) = left_row; //x 좌표
	*(motion_vector + 1) = top_cul; //y 좌표
	for (u = top_cul; u <= bottom_cul; u++) {
		for (v = left_row; v <= right_row; v++) {
			if (min >= *(energy + (u - top_cul)*(right_row - left_row + 1) + (v - left_row))) {
				min = *(energy + (u - top_cul)*(right_row - left_row + 1) + (v - left_row));
				*(motion_vector) = v;
				*(motion_vector + 1) = u;
				//printf("x = %d, y = %d\n", *(motion_vector), *(motion_vector + 1));//test 용======================
			}
		}
	}
	//printf("x = %d, y = %d\n", *(motion_vector), *(motion_vector + 1));//test 용======================
	// 가장 energy 가 작은 블록을 찾아, 그 블록에서 현재 블록의 위치 까지의 모션벡터를 구한다.
	//printf("0번째 에너지 = %d, 1번째 에너지 = %d \n", *(energy), *(energy + 1));
	//printf("dv\n");//test 용====================
	free(energy);
	free(block_pre);
	free(block);
	return  motion_vector;
	free(motion_vector);

}

//decoding
unsigned char* decoding_tempo(unsigned char* before_dec_img, int* mv_x, int* mv_y, int* Q_diff) {
	int i, j;
	int u, v;
	int * invQ_difference = NULL;
	unsigned char * reconstruct_img = NULL;
	unsigned char * block_pre = NULL;
	invQ_difference = (int*)malloc(sizeof(int)*(org_row * org_col));
	reconstruct_img = (unsigned char*)malloc(sizeof(unsigned char)*(org_row * org_col));
	block_pre = (unsigned char*)malloc(sizeof(unsigned char)*(N_tempo*N_tempo));

	invQ_difference = sampling_error(Q_diff, 1, org_row, org_col); //역양자화

	for (i = 0; i < org_col; i += N_tempo) {
		for (j = 0; j < org_row; j += N_tempo) {
			//printf("i =  %d, j = %d\n",i, j);//test 용===========
			block_pre = pre_block_tempo(before_dec_img, i, j, *(mv_x + i / N_tempo*(org_row / N_tempo) + j / N_tempo), *(mv_y + i / N_tempo*(org_row / N_tempo) + j / N_tempo));
			for (u = 0; u < N_tempo; u++) {
				for (v = 0; v < N_tempo; v++) {

					int temp;
					temp = *(block_pre + u*N_tempo + v) + *(invQ_difference + (i + u)*org_row + (j + v));
					if (temp < 0)
						*(reconstruct_img + (i + u)*org_row + (j + v)) = 0;
					else if (temp > 255)
						*(reconstruct_img + (i + u)*org_row + (j + v)) = 255;
					else
						*(reconstruct_img + (i + u)*org_row + (j + v)) = temp;
				}
			}
		}
	}

	free(block_pre);
	free(invQ_difference);
	return reconstruct_img;


	free(reconstruct_img);
}

//encoder 
unsigned char* encoding_tempo(unsigned char* current_img, unsigned char* before_dec_img) {
	int i, j;
	int u, v;
	unsigned char* block_pre = NULL;
	unsigned char * reconstruct_img = NULL;
	unsigned char * pre_img = NULL;//test 용===========
	int * mv_x = NULL;
	int * mv_y = NULL;
	int * a_mv = NULL;
	int * difference = NULL;

	block_pre = (unsigned char*)malloc(sizeof(unsigned char)*(N_tempo*N_tempo));
	reconstruct_img = (unsigned char*)malloc(sizeof(unsigned char)*(org_row * org_col));
	pre_img = (unsigned char*)malloc(sizeof(unsigned char)*(org_row * org_col));//test 용=============
	mv_x = (int*)malloc(sizeof(int)*(org_row * org_col / (N_tempo*N_tempo)));
	mv_y = (int*)malloc(sizeof(int)*(org_row * org_col / (N_tempo*N_tempo)));
	a_mv = (int*)malloc(sizeof(int) * 2);
	difference = (int*)malloc(sizeof(int)*(org_row * org_col));


	for (i = 0; i < org_col; i += N_tempo) {
		for (j = 0; j < org_row; j += N_tempo) {
			//printf("111\n");//test 용====================
			a_mv = mv_finder(current_img, before_dec_img, i, j);
			*(mv_x + (i / N_tempo)*(org_row / N_tempo) + (j / N_tempo)) = *(a_mv);
			*(mv_y + (i / N_tempo)*(org_row / N_tempo) + (j / N_tempo)) = *(a_mv + 1);
			block_pre = pre_block_tempo(before_dec_img, i, j, *(a_mv), *(a_mv + 1));
			//printf("222\n");//test 용====================
			for (u = 0; u < N_tempo; u++) {
				for (v = 0; v < N_tempo; v++) {
					*(difference + (i + u)*org_row + (j + v)) = *(current_img + (i + u)*org_row + (j + v)) - *(block_pre + u*N_tempo + v);
					*(pre_img + (i + u)*org_row + (j + v)) = *(block_pre + u*N_tempo + v);//test 용===============
				}
			}
		}
	}
	//printf("333\n");//test 용====================
	difference = sampling_error(difference, 0, org_row, org_col);
	//printf("444\n");//test 용====================
	reconstruct_img = decoding_tempo(before_dec_img, mv_x, mv_y, difference);
	//printf("555\n");//test 용====================
	WriteFile_I(mv_x, MV_x_file, org_row / N_tempo, org_col / N_tempo);//motion vector 의 x값만 저장하기
	WriteFile_I(mv_y, MV_y_file, org_row / N_tempo, org_col / N_tempo);//motion vector 의 y값만 저장하기
	WriteFile_I(difference, Quantization_file, org_row, org_col);//differency block 저장하기
	WriteFile_U(pre_img, prediction_defore , org_row, org_col);
	free(mv_x);
	free(mv_y);
	free(a_mv);
	free(difference);
	free(block_pre);
	free(pre_img);//test 용=================
	return reconstruct_img;
	free(reconstruct_img);
}


double sum_of_mv(int* mv_x, int* mv_y) {
	double sum = 0;
	int x_2;
	int y_2;
	int i, j;
	for (i = 0; i < org_col; i += N_tempo) {
		for (j = 0; j < org_row; j += N_tempo) {
			x_2 = (*(mv_x + i / N_tempo*(org_row / N_tempo) + j / N_tempo))*(*(mv_x + i / N_tempo*(org_row / N_tempo) + j / N_tempo));
			y_2 = (*(mv_y + i / N_tempo*(org_row / N_tempo) + j / N_tempo))*(*(mv_y + i / N_tempo*(org_row / N_tempo) + j / N_tempo));
			sum += sqrt(x_2 + y_2);
		}
	}
	return sum;
}

//main 함수 
int main(void) {
	unsigned char* buff_img = NULL;
	unsigned char* encoding_current = NULL;
	unsigned char* recontruct_before = NULL;
	int * mv_x = NULL;
	int * mv_y = NULL;
	double sumofmv;
	clock_t before1, after1, before2, after2,before, after;
	double Time1, Time2, total_Time;
	buff_img = (unsigned char*)malloc(sizeof(unsigned char)*(org_row*org_col));
	encoding_current = (unsigned char*)malloc(sizeof(unsigned char)*(org_row*org_col));
	mv_x = (int*)malloc(sizeof(int)*(org_row * org_col / (N_tempo*N_tempo)));
	mv_y = (int*)malloc(sizeof(int)*(org_row * org_col / (N_tempo*N_tempo)));
	recontruct_before = (unsigned char*)malloc(sizeof(unsigned char)*(org_col*org_row));

	before = clock();
	before1 = clock();
	buff_img = readFile(in_file_bf, org_row, org_col);
	recontruct_before = encoding_intra(buff_img); // encoding : 원본 영상을 받아 error 와 label 출력

	after1 = clock();
	Time1 = (double)(after1 - before1);

	before2 = clock();
	buff_img = readFile(in_file_af, org_row, org_col);
	encoding_current = encoding_tempo(buff_img, recontruct_before);
	after2 = clock();
	after = clock();
	Time2 = (double)(after2 - before2);
	total_Time = (double)(after - before);
	//encoding_current = readFile(reconstruct_file_bf, org_row, org_col);

	WriteFile_U(encoding_current, current_encoding_file, org_row, org_col);

	if (use_psnr == 1) {
		printf("===========before image intra prediction===========");
		MSE_f(recontruct_before, in_file_bf); // PSNR, MSE 계산, in_file과 비교
		printf("===========after image temporal prediction===========");
		MSE_f(encoding_current, in_file_af); // PSNR, MSE 계산, in_file과 비교

	}

	mv_x = ReadFile_int(MV_x_file, org_row / N_tempo, org_col / N_tempo);
	mv_y = ReadFile_int(MV_y_file, org_row / N_tempo, org_col / N_tempo);
	sumofmv = sum_of_mv(mv_x, mv_y);
	printf("MV들의 절대값의 sum : %f", sumofmv);

	printf("\ndefore image encoding time : %.4f (msec)\n", Time1);
	printf("\ncurrent image encoding time : %.4f (msec)\n", Time2);
	printf("\n encoding time : %.4f (msec)\n", total_Time);
	free(buff_img);
	free(mv_x);
	free(mv_y);
	free(encoding_current);
	free(recontruct_before);
	return 0;
}