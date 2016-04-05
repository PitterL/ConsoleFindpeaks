// ConsoleFindpeaks.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <iostream>
#include <vector>

using namespace std;

struct peak{
	int val;	//peak value
	int index;	//peak index number
	int sign;	//peak type: value of second derivative, determinate maximum or minimum
};
typedef struct peak v_peak;

typedef unsigned char u8;
struct position{
	u8 index;	//channel number integer
	u8 index_decimal;	//chnnel number decimal
	int val;	//value
};

enum{
	START,
	END,
	RANGE
};

struct hamp{
	vector<struct peak>::iterator full[RANGE];	//store index range of the hamp
	struct position point;
};
typedef struct hamp v_hamp;

struct merge_condition{
	int thld;	//condition 2: positive peak value must less than threshold 
	int hysis;	//condition 1: to adjcent high peak less than hysis
	int count;	//condition 1: OR two positive peak distance less than count
};

struct peak_condition{
	int thld;	//valid peak delta
	int hysis;	//minimum value change for calculate first derivative  
};

//diff will ignore hysis ripple
int inline caculate_diff(int val, int prev, int hysis)
{
	int diff;

	diff = val - prev;
	if (abs(diff) < hysis)
		diff = 0;

	return diff;
}

typedef int peak_buff, *peak_buff_t;
/*
	findPeaks:  find all the maximum and minimum in the given array 'buf'
		[buf]: raw data
		[count]: raw data length
		[hysis]: the jitter value of raw data, if diff of two data is less than hysis, we think about it equation
		[vec_peak]: output peak to the vector
*/
void findPeaks(peak_buff_t buf, int count, int hysis, vector<struct peak> &vector_peak)
{
	/*	
		Definition for calculating first derivative value:
					diff[0]: buf[i] - buf[i - 1]
					diff[1]: buf[i + 1] - buf[i]
		i: the index number of current element
		j: offset add to index i to calculate diff[0]:	
			buf[i] - buf[i - 1] => 
				buf[i + j] - buf[i - 1]
		k: offset add to index i + 1 to calculate diff[1]: 
			buf[i + 1] - buf[i] => 
				buf[i + k] - buf[i]
	*/
	int i, j, k;
	int diff[2];
	v_peak val_p;

	for (i = 1, j = 0, k = 1; i + k < count;)	{

		//WARNING_ON(j >= k);

		//calculate fist derivative
		diff[0] = caculate_diff(buf[i + j], buf[i - 1], hysis);
		diff[1] = caculate_diff(buf[i + k], buf[i + j], hysis);

		//printf("[%d %d %d (%d %d %d)] = %d %d %d [%d %d]\n", i - 1, i + j, i + k, i, j, k, buf[i - 1], buf[i + j], buf[i + k], diff[0], diff[1]);

		if (diff[0] && diff[1]) {
			if ((diff[0] ^ diff[1]) < 0) {   //first derivative opposite sign, there will be a maximum/minimum
				val_p.index = i + j /*+ ((k-j) >> 1)*/;	//if index use i + j: first index of maximum/minimum in same value
													//if index use j + j + ((k-j)>>1): middle index of maximum/minimus in same value
				val_p.val = buf[val_p.index];
				val_p.sign = diff[1] - diff[0];  //second derivative
				vector_peak.push_back(val_p);

				//printf("*%d: %d %s\n", val_p.index, val_p.val, val_p.sign > 0 ? "MIN":"MAX");
			}

			//move index to next element(which it's next one following the index i + j)
			i += j + 1;	//combine j to i, so k will be re-calculated for i + k offset same
			k -= j;
			j = 0;
		}
		else {
			//some derivative is zero, so we should continue to search next non-zero value
			if (!diff[0]) {		//re-search valid diff[0]
				for (j = 1 ; i + j < count - 1; j++) {		//search diff[0], the index start at (i + j' + 1), and end before the last element
					diff[0] = caculate_diff(buf[i + j], buf[i - 1], hysis);
					if (diff[0])
						break;
				}
			}

			if (diff[0]) {		//re-search valid diff[1]
				for (k = j + 1; i + k < count; k++) {	//search diff[1], the index start at (i + j'' + 1), and end at last element
					diff[1] = caculate_diff(buf[i + k], buf[i + j], hysis);
					if (diff[1])
						break;
				}
			}

			if (!diff[0] || !diff[1])	//no valid derivative could be found
				break;
		}
	}
}
/*
	append_peaks_in_edge: we get local maximum and minimum at open interval(a,b), 
							and we should consider about the close interval [a,b], so we append the value in point a and b 
		[buf]: raw data
		[thld]: raw data length
		[thld]: when we consider about first derivative of the end point, only abs(diff) >= thld will be consider as valid value
		[vec_peak]: output peak to the vector
*/
void append_peaks_in_edge(peak_buff_t buf, int count, int thld, vector<struct peak> &vector_peak)
{
	vector<struct peak>::iterator iter;
	vector<struct peak> vector_dummy;
	v_peak val_p, dummy_min = {/*.index =*/ -1, /*.sign =*/ 1, /*.val =*/ 0,};
	bool empty = vector_peak.empty();

	vector_dummy.push_back(dummy_min);
	//add peak in front
	if (empty)
		iter = vector_dummy.begin();
	else
		iter = vector_peak.begin();
	if (iter->index != 0) {
		val_p.index = 0;
		val_p.val = buf[val_p.index];
		if (iter->sign > 0 &&
			iter->val + thld <= buf[0]) {	//first element is valid minimum, we mark point 'a' as maximum
			val_p.sign = -1;	//dummy second derivative
		}
		else if (iter->sign < 0 &&
			iter->val - thld >= buf[0]) {	//first element is valid maximum, we mark point 'a' as minimum
			val_p.sign = 1;
		}
		else {
			val_p.sign = 0;	//invalid point 'a'
		}
		if (val_p.sign)
			vector_peak.insert(vector_peak.begin(), val_p);
	}

	//add peak in end
	if (empty)
		iter = vector_dummy.begin();
	else
		iter = vector_peak.end() - 1;
	if (iter->index != count - 1) {
		val_p.index = count - 1;
		val_p.val = buf[val_p.index];
		
		if (iter->sign > 0 &&
			iter->val + thld <= buf[count - 1]) {	//last element is valid minimum, we mark point 'b' as maximum
			val_p.sign = -1;
		}
		else if (iter->sign < 0 &&
			iter->val - thld >= buf[count - 1]) {	//last element is valid maximum, we mark point 'b' as minimum
			val_p.sign = 1;
		}
		else{
			val_p.sign = 0;	//invalid point 'b'
		}
		if (val_p.sign)
			vector_peak.push_back(val_p);
	}
}

/*
	merge_adjacent_high_peak:	merge adjacent maximum value
		[vec_peak]: maximum and minimum vector
		[thld]: peak val >= thld will be consider as valid peak and store to hamp vector
		[mcfg]: merge necessary condition: <1> distance < merge count OR two diff(max - max) < hysis <2> diff(max - min) < merge threshold
		[vec_hamp]:	output merged peak to hamp vector
*/
void merge_adjacent_high_peak(vector<struct peak> &vector_peak, int thld, const struct merge_condition &mcfg, vector<struct hamp> &vector_hamp)
{
	/* iter##i:
	0 : first high peak
	1 : second high peak
		2 : low peak after iter0
		3 : low peak before iter1, --- it may same number iter2
		start: hamp start position
		end: hamp end position
	*/
	vector<struct peak>::iterator iter0, iter1, iter2, iter3, iter_end, iter_start;
	int distance,diff;
	v_hamp h_val;

	for (iter0 = vector_peak.begin(); vector_peak.size() && iter0 <= vector_peak.end() - 1;) {	//iter0, first maximum

		if (iter0->sign < 0 && iter0->val > thld) {		//check maximum threshold
			iter_start = iter_end = iter0;

			for (iter1 = iter0 + 1; iter1 <= vector_peak.end() - 1; iter1++) {
				if (iter1->sign < 0 && iter1->val > thld) {		//check maximum threshold
					distance = iter1->index - iter0->index;	//it's a positive integer
					diff = iter0->val - iter1->val;
					diff = abs(diff);
					if (distance < mcfg.count || diff < mcfg.hysis) {	//first necessary merge condition: distance < merge count OR two diff(max - max) < hysis
						iter2 = iter0 + 1;	//	minimum after iter0
						iter3 = iter1 - 1;	//	minimum before iter1

						if (iter2->sign > 0 && iter3->sign > 0) {	//these two value must > 0, it's minimum
							if (iter0->val - iter2->val < mcfg.thld ||
								iter1->val - iter3->val < mcfg.thld) {	//second necessary merge condition: diff(max - min) < merge threshold
								iter_end = iter1;
							}
							else
								break;	// merge condition 2 not matched
						}else {
							//WARN_ON(1);
							break;	// something error happen
						}
					}
					else
						break;	// merge condition 1 not matched
				}
			}

			//set range to minimum
			if (iter_start > vector_peak.begin())
				iter_start--;
			if (iter_end < vector_peak.end() - 1)
				iter_end++;

			//memset(&h_val, 0, sizeof(h_val));
			memset(&h_val.point, 0, sizeof(h_val.point));
			h_val.full[START] = iter_start;
			h_val.full[END] = iter_end;
			vector_hamp.push_back(h_val);

			iter0 = iter1;
		}
		else
			iter0++;
	}
}

//range find: from first data large than threshold to last data large than threshold
int cacluate_range(const peak_buff_t buf, int st, int len, int thld)
{
	int i, j;

	for (i = 0; i < len; i++) {
		if (buf[st + i] > thld) {
			for (j = len - 1; j >= i; j--) {
				if (buf[st + j] > thld) {
					return (((st + i) << 16) | (j - i + 1));
				}
			}
		}
	}

	return 0;
}

//average value
int caculate_average_value(const peak_buff_t buf, int st, int len)
{
	int i, sum, avg;

	sum = avg = 0;
	for (i = 0; i < len; i++) {
		sum += buf[st + i];
	}

	if (sum) {
		avg = sum / len;
	}

	return avg;
}

//variance value
int caculate_variance_value(const peak_buff_t buf, int st, int len, int avg)
{
	int i, e_1, e_sum;

	avg = caculate_average_value(buf, st, len);

	e_sum = 0;
	for (i = 0; i < len; i++) {
		e_1 = buf[st + i] - avg;
		e_1 *= e_1;
		e_sum += e_1;
	}

	return e_sum;
}

//position: from middle, check strength left and right, calculate percent 
void caculate_position(const peak_buff_t buf, int st, int len, struct position &pos)
{
	int i, mid, sum[2], percent;

	pos.index = 0;
	pos.index_decimal = 0;
	pos.val = 0;

	if (len == 1) {
		pos.val = buf[st];
		pos.index = st;
	}else {
		sum[0] = sum[1] = 0;
		mid = len >> 1;
		for (i = 0; i < mid; i++) {
			sum[0] += buf[st + i];
			sum[1] += buf[st + len - i - 1];
		}
		if (sum[0] + sum[1]) {
			percent = (sum[0] * 100) / (sum[0] + sum[1]);
			pos.val = (buf[st + mid - 1] * percent + buf[st + ((len + 1) >> 1)] * (100 - percent)) / 100;
			pos.index = st + mid - 1;
			pos.index_decimal = 100 - percent;
		}
	}
}

//search where is the middle point of each hamp
/*  1 calculate range which data value higher than average value
	2 higher average value
	3 calculate again
	exit condition: <1> array len less than 3 <2> average steady <3>  variance less than hysis */
void caculate_peak_in_hamp(peak_buff_t buf, vector<struct hamp> &vector_hamp, int thld, int hysis)
{
	vector<struct hamp>::iterator h_iter;
	struct position pos;
	int val, st[2], len[2], avg[2], avg_diff, e_sum, e_diff;

	for (h_iter = vector_hamp.begin(); vector_hamp.size() && h_iter <= vector_hamp.end() - 1; h_iter++) {

		st[0] = st[1] = h_iter->full[START]->index;
		len[0] = len[1] = h_iter->full[END]->index - st[1] + 1;
		avg[0] = avg[1] = thld;

		do {
			val = cacluate_range(buf, st[1], len[1], avg[1]);
			if (val) {
				avg[0] = avg[1];
				st[0] = st[1];
				len[0] = len[1];

				st[1] = (val >> 16);
				len[1] = val & 0xffff;
				avg[1] = caculate_average_value(buf, st[1], len[1]);
				e_sum = caculate_variance_value(buf, st[1], len[1], avg[1]);

				avg_diff = avg[0] - avg[1];
				e_diff = hysis * hysis;

				//printf("%d %d: %d(%d, %d) %d\n", st[1], len[1], avg_diff, avg[0], avg[1], e_sum);
				if (len[1] <= 3 || e_sum <= e_diff) {
					st[0] = st[1];
					len[0] = len[1];
					break;
				}
			}else
				break;
		} while (avg_diff < -hysis);

		caculate_position(buf, st[0], len[0], pos);
		h_iter->point = pos;
		//printf("point(%d.%d) = %d\n", pos.index, pos.index_decimal, pos.val);
	}
}

void debug_output_peak_vector(vector<struct peak> &vector_peak)
{
	for (vector<struct peak>::iterator iter = vector_peak.begin();
		vector_peak.size() && iter <= vector_peak.end() - 1; iter++) {
		printf("%d: %d ", iter->index, iter->val);
		if (iter->sign < 0)
			printf("MAX");
		else if (iter->sign > 0)
			printf("MIN");
		else
			printf("--");
		printf("\n");
	}
}

void debug_output_peak_hamp(vector<struct hamp> &vector_hamp)
{
	for (vector<struct hamp>::iterator iter = vector_hamp.begin();
		vector_hamp.size() && iter <= vector_hamp.end() - 1; iter++) {
		printf("%d (%d.%d), [%d] %d ~ [%d] %d\n", 
			iter->point.val, iter->point.index, iter->point.index_decimal, 
			iter->full[START]->index, iter->full[START]->val, 
			iter->full[END]->index,iter->full[END]->val);
	}
}

int _tmain(int argc, _TCHAR* argv[])
{
	vector<struct peak> vector_peak;
	vector<struct hamp> vector_hamp;
	int num;
	peak_buff a[] = {16,15,-1, 2, 2, 5, 7,10,12,15,13,8,7,-1, 2, 3, -4,  2, 2 , 1, 11, 4, 1, 8, 7, 9,12, 23 ,24, 25, 23,24,24,24,24,24,23,24,23,24,25,20,21};
	int thld = 5;
	int hysis = 2;
	struct merge_condition merge_conf = {5, 3, 2};

	num = sizeof(a) / sizeof(a[0]);
	for (int m = 0; m < num; m++)
		cout << a[m] << " ";
	cout << endl;

	findPeaks(a, num, hysis, vector_peak);
	cout << "peak:" << endl;
	debug_output_peak_vector(vector_peak);
	
	append_peaks_in_edge(a, num, thld, vector_peak);
	cout << "peak(merged):" << endl;
	debug_output_peak_vector(vector_peak);

	merge_adjacent_high_peak(vector_peak, thld, merge_conf, vector_hamp);
	cout << "hamp:" << endl;
	debug_output_peak_hamp(vector_hamp);

	caculate_peak_in_hamp(a, vector_hamp, thld, hysis);
	cout << "hamp(2):" << endl;
	debug_output_peak_hamp(vector_hamp);

	return 0;
}
