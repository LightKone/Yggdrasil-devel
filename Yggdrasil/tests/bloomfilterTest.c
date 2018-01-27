/*********************************************************
 * This code was written in the context of the Lightkone
 * European project.
 * Code is of the authorship of NOVA (NOVA LINCS @ DI FCT
 * NOVA University of Lisbon)
 * Authors:
 * Pedro Ákos Costa (pah.costa@campus.fct.unl.pt)
 * João Leitão (jc.leitao@fct.unl.pt)
 * (C) 2017
 *********************************************************/

#include "lightkone.h"
#include "core/utils/queue.h"
#include "core/lk_runtime.h"
#include "protos/discovery/discoveryFullFT.h"
#include "protos/faultdetection/faultdetector.h"
#include "core/utils/utils.h"

#include "core/utils/bloomfilter/bloom.h"
#include "core/utils/hashfunctions.h"

#include <stdlib.h>


int main(int argc, char* argv[]) {

	bloom_t filter = bloom_create(1300);
	bloom_add_hash(filter, &DJBHash);
	bloom_add_hash(filter, &JSHash);
	bloom_add_hash(filter, &RSHash);

	int counter = 1;
	int i;

	printf("PUT ALL, TEST ALL\n");

	uuid_t id[10];

	for(i = 0; i < 10; i++) {

		char* uuid_txt = malloc(37);
		memset(uuid_txt, 0, 37);
		sprintf(uuid_txt, "66600666-1001-1001-1001-0000000000%.02d", i);
		if(uuid_parse(uuid_txt, id[i]) != 0)
			genUUID(id);

		bloom_add(filter, id[i], sizeof(uuid_t));
		printf("Addded %s to bloom filter\n", uuid_txt);
		free(uuid_txt);
	}

	for(i = 0; i < 10; i++) {

		int r = bloom_test(filter, id[i], sizeof(uuid_t));
		printf("Tested uuid %d bloom filter result %d\n", i, r);
	}

	bloom_t filter2 = bloom_create(1300);
	bloom_add_hash(filter2, &DJBHash);
	bloom_add_hash(filter2, &JSHash);
	bloom_add_hash(filter2, &RSHash);

	bloom_t filter3 = bloom_create(1300);
	bloom_add_hash(filter3, &DJBHash);
	bloom_add_hash(filter3, &JSHash);
	bloom_add_hash(filter3, &RSHash);

	int r = bloom_test_disjoin(filter, bloom_getBits(filter2), bloom_getSize(filter2));

	printf("filter 1 and 2 are disjoin (should be true 1) %d\n", r);

	r = bloom_test_equal(filter, bloom_getBits(filter2), bloom_getSize(filter2));

	printf("filter 1 and 2 are equal (should be false 0) %d\n", r);

	r = bloom_test_equal(filter2, bloom_getBits(filter3), bloom_getSize(filter3));

	printf("filter 2 and 3 are equal (should be true 1) %d\n", r);

	r = bloom_test_disjoin(filter2, bloom_getBits(filter3), bloom_getSize(filter3));

	printf("filter 2 and 3 are disjoin (should be true 1) %d\n", r);


	bloom_free(filter);

	filter = bloom_create(1300);
	bloom_add_hash(filter, &DJBHash);
	bloom_add_hash(filter, &JSHash);
	bloom_add_hash(filter, &RSHash);


	printf("PUT SAME, TEST ALL\n");

	for(i = 0; i < 10; i++){
		bloom_add(filter, id[0], sizeof(uuid_t));
	}

	for(i = 0; i < 10; i++) {

		r = bloom_test(filter, id[i], sizeof(uuid_t));
		printf("Tested uuid %d bloom filter result %d\n", i, r);
	}

	bloom_free(filter);

	filter = bloom_create(1300);
	bloom_add_hash(filter, &DJBHash);
	bloom_add_hash(filter, &JSHash);
	bloom_add_hash(filter, &RSHash);


	bloom_add(filter, id[0], sizeof(uuid_t));

	printf("PUT ONE, TEST ALL\n");

	for(i = 0; i < 10; i++) {

		r = bloom_test(filter, id[i], sizeof(uuid_t));
		printf("Tested uuid %d bloom filter result %d\n", i, r);
	}

	printf("TWO SETS, ONE ELEMENT\n");


	bloom_free(filter2);

	filter2 = bloom_create(1300);
	bloom_add_hash(filter2, &DJBHash);
	bloom_add_hash(filter2, &JSHash);
	bloom_add_hash(filter2, &RSHash);

	bloom_add(filter2, id[0], sizeof(uuid_t));

	bloom_free(filter3);

	filter3 = bloom_create(1300);
	bloom_add_hash(filter3, &DJBHash);
	bloom_add_hash(filter3, &JSHash);
	bloom_add_hash(filter3, &RSHash);

	bloom_add(filter3, id[1], sizeof(uuid_t));

	r = bloom_test_disjoin(filter, bloom_getBits(filter2), bloom_getSize(filter2));

	printf("filter 1 and 2 are disjoin (should be false 0) %d\n", r);

	r = bloom_test_disjoin(filter, bloom_getBits(filter3), bloom_getSize(filter3));

	printf("filter 1 and 3 are disjoin (should be true 1) %d\n", r);

	r = bloom_test_equal(filter, bloom_getBits(filter2), bloom_getSize(filter2));

	printf("filter 1 and 2 are equal (should be true 1) %d\n", r);

	r = bloom_test_equal(filter, bloom_getBits(filter3), bloom_getSize(filter3));

	printf("filter 1 and 3 are equal (should be false 0) %d\n", r);

	printf("INVERTING TEST\n");

	r = bloom_test_disjoin(filter2, bloom_getBits(filter), bloom_getSize(filter));

	printf("filter 2 and 1 are disjoin (should be false 0) %d\n", r);

	r = bloom_test_disjoin(filter3, bloom_getBits(filter), bloom_getSize(filter));

	printf("filter 3 and 1 are disjoin (should be true 1) %d\n", r);

	r = bloom_test_equal(filter2, bloom_getBits(filter), bloom_getSize(filter));

	printf("filter 2 and 1 are equal (should be true 1) %d\n", r);

	r = bloom_test_equal(filter3, bloom_getBits(filter), bloom_getSize(filter));

	printf("filter 3 and 1 are equal (should be false 0) %d\n", r);

	bloom_swap(filter, bloom_getBits(filter2), bloom_getSize(filter2));

	bloom_add(filter2, id[3], sizeof(uuid_t));

	printf("TESTING ELEMS AFTER SWAP only 0 should true\n");

	for(i = 0; i < 10; i++) {

		r = bloom_test(filter, id[i], sizeof(uuid_t));
		printf("Tested uuid %d bloom filter result %d\n", i, r);
	}


	bloom_merge(filter, bloom_getBits(filter3), bloom_getSize(filter3));

	bloom_add(filter3, id[3], sizeof(uuid_t));

	printf("TESTING ELEMS AFTER MERGE, 0 and 1 should be true\n");

	for(i = 0; i < 10; i++) {

		r = bloom_test(filter, id[i], sizeof(uuid_t));
		printf("Tested uuid %d bloom filter result %d\n", i, r);
	}

	bloom_free(filter);
	bloom_free(filter2);
	bloom_free(filter3);


	filter2 = bloom_create(1300);
	bloom_add_hash(filter2, &DJBHash);
	bloom_add_hash(filter2, &JSHash);
	bloom_add_hash(filter2, &RSHash);

	for(i = 0; i < 5; i++){
		bloom_add(filter2, id[i], sizeof(uuid_t));
	}

	filter3 = bloom_create(1300);
	bloom_add_hash(filter3, &DJBHash);
	bloom_add_hash(filter3, &JSHash);
	bloom_add_hash(filter3, &RSHash);


	for(i = 5; i < 10; i++){
		bloom_add(filter3, id[i], sizeof(uuid_t));
	}

	printf("TEST DISJOIN FULL\n");

	r = bloom_test_disjoin(filter2, bloom_getBits(filter3), bloom_getSize(filter3));

	printf("filter 2 and 3 are disjoin (should be true 1) %d\n", r);

	r = bloom_test_disjoin(filter3, bloom_getBits(filter2), bloom_getSize(filter2));

	printf("filter 3 and 2 are disjoin (should be true 1) %d\n", r);

	bloom_merge(filter2, bloom_getBits(filter3), bloom_getSize(filter3));

	printf("TESTING ELEMS AFTER FULL MERGE\n");

	for(i = 0; i < 10; i++) {

		r = bloom_test(filter2, id[i], sizeof(uuid_t));
		printf("Tested uuid %d bloom filter result %d\n", i, r);
	}

	bloom_free(filter2);
	bloom_free(filter3);

	filter2 = bloom_create(1300);
	bloom_add_hash(filter2, &DJBHash);
	bloom_add_hash(filter2, &JSHash);
	bloom_add_hash(filter2, &RSHash);

	filter3 = bloom_create(1300);
	bloom_add_hash(filter3, &DJBHash);
	bloom_add_hash(filter3, &JSHash);
	bloom_add_hash(filter3, &RSHash);

	printf("TESTING EQUAL WITH DIFFERENT ADDITION\n");

	for(i = 0; i <= 5; i++){
		bloom_add(filter3, id[i], sizeof(uuid_t));
	}

	for(i = 5; i >= 0; i--){
			bloom_add(filter2, id[i], sizeof(uuid_t));
	}

	r = bloom_test_equal(filter2, bloom_getBits(filter3), bloom_getSize(filter3));

	printf("filter 2 and 3 are equal (should be true 1) %d\n", r);


	return 0;
}

