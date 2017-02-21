/*
 * Tests for PIO data decompositons.
 *
 * Ed Hartnett
 */
#include <pio.h>
#include <pio_tests.h>

/* The number of tasks this test should run on. */
#define TARGET_NTASKS 4

/* The minimum number of tasks this test should run on. */
#define MIN_NTASKS 4

/* The name of this test. */
#define TEST_NAME "test_decomps"

/* The number of dimensions in the example data. In this test, we
 * are using three-dimensional data. */
#define NDIM 3

/* In decompositions we specify with 1 less dimension. */
#define NDIM2 2

/* The length of our sample data along each dimension. */
#define X_DIM_LEN 4
#define Y_DIM_LEN 4

/* The number of timesteps of data to write. */
#define NUM_TIMESTEPS 1

/* Files of decompositions. */
#define DECOMP_FILE "decomp.txt"
#define DECOMP_BC_FILE "decomp.txt"

/* Used when initializing PIO. */
#define STRIDE1 1
#define STRIDE2 2
#define BASE0 0
#define BASE1 1
#define NUM_IO1 1
#define NUM_IO2 2
#define NUM_IO4 4
#define REARRANGER 2

/**
 * Test some decomposition functions. 
 *
 * @param iosysid the IO system ID.
 * @param my_rank the 0-based rank of this task.
 * @param test_comm communicator that includes all tasks paticipating in test.
 * @returns 0 for success, error code otherwise.
 */
int test_decomp1(int iosysid, int my_rank, MPI_Comm test_comm)
{
    int ioid;                   /* The decomposition ID. */
    PIO_Offset elements_per_pe; /* Array index per processing unit. */
    PIO_Offset *compdof;        /* The decomposition mapping. */
    int slice_dimlen[2];
    int bad_slice_dimlen[2];    /* Invalid values. */
    int ndims;
    int *gdims;
    PIO_Offset fmaplen;
    PIO_Offset *map;
    int ret;
    
    /* Describe the decomposition. This is a 1-based array, so add 1! */
    slice_dimlen[0] = X_DIM_LEN;
    slice_dimlen[1] = Y_DIM_LEN;
    elements_per_pe = X_DIM_LEN * Y_DIM_LEN / TARGET_NTASKS;

    /* The compdof array contains a mapping for this task into the
     * global data array. */
    if (!(compdof = malloc(elements_per_pe * sizeof(PIO_Offset))))
        return PIO_ENOMEM;
    for (int i = 0; i < elements_per_pe; i++)
        compdof[i] = my_rank * elements_per_pe + i + 1;

    /* These should not work. */
    bad_slice_dimlen[1] = 0;
    if (PIOc_InitDecomp(iosysid + TEST_VAL_42, PIO_FLOAT, 2, slice_dimlen, (PIO_Offset)elements_per_pe,
                        compdof, &ioid, NULL, NULL, NULL) != PIO_EBADID)
        return ERR_WRONG;
    if (PIOc_InitDecomp(iosysid, PIO_FLOAT, 2, bad_slice_dimlen, (PIO_Offset)elements_per_pe,
                        compdof, &ioid, NULL, NULL, NULL) != PIO_EINVAL)
        return ERR_WRONG;
    if (PIOc_InitDecomp(iosysid, PIO_FLOAT, 2, NULL, (PIO_Offset)elements_per_pe,
                        compdof, &ioid, NULL, NULL, NULL) != PIO_EINVAL)
        return ERR_WRONG;
    if (PIOc_InitDecomp(iosysid, PIO_FLOAT, 2, slice_dimlen, (PIO_Offset)elements_per_pe,
                        NULL, &ioid, NULL, NULL, NULL) != PIO_EINVAL)
        return ERR_WRONG;
    if (PIOc_InitDecomp(iosysid, PIO_FLOAT, 2, slice_dimlen, (PIO_Offset)elements_per_pe,
                        compdof, NULL, NULL, NULL, NULL) != PIO_EINVAL)
        return ERR_WRONG;
        
    /* Create the PIO decomposition for this test. */
    printf("%d Creating decomposition...\n", my_rank);
    if ((ret = PIOc_InitDecomp(iosysid, PIO_FLOAT, 2, slice_dimlen, (PIO_Offset)elements_per_pe,
                               compdof, &ioid, NULL, NULL, NULL)))
        return ret;
    free(compdof);

    /* These should not work. */
    if (PIOc_write_decomp(DECOMP_FILE, iosysid + TEST_VAL_42, ioid, test_comm) != PIO_EBADID)
        return ERR_WRONG;
    if (PIOc_write_decomp(DECOMP_FILE, iosysid, ioid + TEST_VAL_42, test_comm) != PIO_EBADID)
        return ERR_WRONG;

    /* Write the decomp file. */
    if ((ret = PIOc_write_decomp(DECOMP_FILE, iosysid, ioid, test_comm)))
        return ret;

    /* These should not work. */
    if (PIOc_readmap(NULL, &ndims, (int **)&gdims, &fmaplen, (PIO_Offset **)&map,
                     test_comm) != PIO_EINVAL)
        return ERR_WRONG;
    if (PIOc_readmap(DECOMP_FILE, NULL, (int **)&gdims, &fmaplen, (PIO_Offset **)&map,
                     test_comm) != PIO_EINVAL)
        return ERR_WRONG;
    if (PIOc_readmap(DECOMP_FILE, &ndims, NULL, &fmaplen, (PIO_Offset **)&map,
                     test_comm) != PIO_EINVAL)
        return ERR_WRONG;
    if (PIOc_readmap(DECOMP_FILE, &ndims, (int **)&gdims, NULL, (PIO_Offset **)&map,
                     test_comm) != PIO_EINVAL)
        return ERR_WRONG;
    if (PIOc_readmap(DECOMP_FILE, &ndims, (int **)&gdims, &fmaplen, NULL, test_comm) != PIO_EINVAL)
        return ERR_WRONG;

    /* Read the decomp file and check results. */
    if ((ret = PIOc_readmap(DECOMP_FILE, &ndims, (int **)&gdims, &fmaplen, (PIO_Offset **)&map,
                            test_comm)))
        return ret;
    printf("ndims = %d fmaplen = %lld\n", ndims, fmaplen);
    if (ndims != 2 || fmaplen != 4)
        return ERR_WRONG;
    for (int d = 0; d < ndims; d++)
    {
        printf("gdims[%d] = %d\n", d, gdims[d]);
    }
    for (int m = 0; m < fmaplen; m++)
    {
        printf("map[%d] = %lld\n", m, map[m]);
    }

    free(map);
    free(gdims);

    /* These should not work. */
    if (PIOc_freedecomp(iosysid + TEST_VAL_42, ioid) != PIO_EBADID)
        return ERR_WRONG;
    if (PIOc_freedecomp(iosysid, ioid + TEST_VAL_42) != PIO_EBADID)
        return ERR_WRONG;
        
    /* Free the PIO decomposition. */
    printf("%d Freeing PIO decomposition...\n", my_rank);
    if ((ret = PIOc_freedecomp(iosysid, ioid)))
        return ret;
        
    return 0;
}

/**
 * Test PIOc_InitDecomp_bc(). 
 *
 * @param iosysid the IO system ID.
 * @param my_rank the 0-based rank of this task.
 * @param test_comm communicator that includes all tasks paticipating in test.
 * @returns 0 for success, error code otherwise.
 */
int test_decomp_bc(int iosysid, int my_rank, MPI_Comm test_comm)
{
    int ioid;                   /* The decomposition ID. */
    long int start[NDIM2];
    long int count[NDIM2];
    long int bad_count[NDIM2] = {-1, 0};
    long int bad_start[NDIM2] = {-1, 0};
    int ndims;
    int *gdims;
    PIO_Offset fmaplen;
    PIO_Offset *map;
    int slice_dimlen[NDIM2];
    int ret;
    
    /* Describe the decomposition. This is a 1-based array, so add 1! */
    start[0] = my_rank;
    start[1] = 0;
    count[0] = 1;
    count[1] = 4;
    slice_dimlen[0] = X_DIM_LEN;
    slice_dimlen[1] = Y_DIM_LEN;

    /* These should not work. */
    if (PIOc_InitDecomp_bc(iosysid + TEST_VAL_42, PIO_FLOAT, 2, slice_dimlen, start, count, &ioid) != PIO_EBADID)
        return ERR_WRONG;
    printf("ret = %d\n", PIOc_InitDecomp_bc(iosysid, PIO_FLOAT, 2, NULL, start, count, &ioid));
    if (PIOc_InitDecomp_bc(iosysid, PIO_FLOAT, 2, NULL, start, count, &ioid) != PIO_EINVAL)
        return ERR_WRONG;
    if (PIOc_InitDecomp_bc(iosysid, PIO_FLOAT, 2, slice_dimlen, NULL, count, &ioid) != PIO_EINVAL)
        return ERR_WRONG;
    if (PIOc_InitDecomp_bc(iosysid, PIO_FLOAT, 2, slice_dimlen, start, NULL, &ioid) != PIO_EINVAL)
        return ERR_WRONG;
    if (PIOc_InitDecomp_bc(iosysid, PIO_FLOAT, 2, slice_dimlen, start, count, NULL) != PIO_EINVAL)
        return ERR_WRONG;
    if (PIOc_InitDecomp_bc(iosysid, PIO_FLOAT, 2, slice_dimlen, bad_start, count, &ioid) != PIO_EINVAL)
        return ERR_WRONG;
    if (PIOc_InitDecomp_bc(iosysid, PIO_FLOAT, 2, slice_dimlen, start, bad_count, &ioid) != PIO_EINVAL)
        return ERR_WRONG;
        
    /* Create the PIO decomposition for this test. */
    printf("%d Creating decomposition...\n", my_rank);
    if ((ret = PIOc_InitDecomp_bc(iosysid, PIO_FLOAT, 2, slice_dimlen, start, count, &ioid)))
        return ret;

    /* Write the decomp file. */
    if ((ret = PIOc_write_decomp(DECOMP_BC_FILE, iosysid, ioid, test_comm)))
        return ret;

    /* Read the decomp file and check results. */
    if ((ret = PIOc_readmap(DECOMP_BC_FILE, &ndims, (int **)&gdims, &fmaplen, (PIO_Offset **)&map,
                            test_comm)))
        return ret;
    printf("ndims = %d fmaplen = %lld\n", ndims, fmaplen);
    if (ndims != 2 || fmaplen != 4)
        return ERR_WRONG;
    for (int d = 0; d < ndims; d++)
    {
        printf("gdims[%d] = %d\n", d, gdims[d]);
    }
    for (int m = 0; m < fmaplen; m++)
    {
        printf("map[%d] = %lld\n", m, map[m]);
    }

    free(map);
    free(gdims);
        
    /* Free the PIO decomposition. */
    printf("%d Freeing PIO decomposition...\n", my_rank);
    if ((ret = PIOc_freedecomp(iosysid, ioid)))
        return ret;
        
    return 0;
}

/* Run async tests. */
int main(int argc, char **argv)
{
    int my_rank; /* Zero-based rank of processor. */
    int ntasks;  /* Number of processors involved in current execution. */
    int iosysid; /* The ID for the parallel I/O system. */
    MPI_Group world_group;      /* An MPI group of world. */
    MPI_Comm test_comm;
    int num_flavors;            /* Number of PIO netCDF flavors in this build. */
    int flavor[NUM_FLAVORS];    /* iotypes for the supported netCDF IO flavors. */
    int ret;                    /* Return code. */

    /* Initialize test. */
    if ((ret = pio_test_init(argc, argv, &my_rank, &ntasks, TARGET_NTASKS,
                             &test_comm)))
        ERR(ERR_INIT);

    /* Test code runs on TARGET_NTASKS tasks. The left over tasks do
     * nothing. */
    if (my_rank < TARGET_NTASKS)
    {
        /* Figure out iotypes. */
        if ((ret = get_iotypes(&num_flavors, flavor)))
            ERR(ret);

        /* Initialize PIO system on world. */
        printf("%d about to call Init_Intracomm\n", my_rank);
        if ((ret = PIOc_Init_Intracomm(test_comm, NUM_IO4, STRIDE1, BASE0, REARRANGER, &iosysid)))
            ERR(ret);
        printf("%d done with Init_Intracomm\n", my_rank);

        /* Set the error handler. */
        /*PIOc_Set_IOSystem_Error_Handling(iosysid, PIO_BCAST_ERROR);*/
        printf("%d about to set iosystem error hanlder for world\n", my_rank);
        if ((ret = PIOc_set_iosystem_error_handling(iosysid, PIO_BCAST_ERROR, NULL)))
            ERR(ret);
        printf("%d done setting iosystem error hanlder for world\n", my_rank);

        /* Get MPI_Group of world comm. */
        if ((ret = MPI_Comm_group(test_comm, &world_group)))
            ERR(ret);

        /* Test basic decomp stuff. */
        if ((ret = test_decomp1(iosysid, my_rank, test_comm)))
            return ret;

        /* Test PIOc_InitDecomp_bc(). */
        if ((ret = test_decomp_bc(iosysid, my_rank, test_comm)))
            return ret;

        /* Finalize PIO systems. */
        printf("%d pio finalized\n", my_rank);
        if ((ret = PIOc_finalize(iosysid)))
            ERR(ret);

        /* Free MPI resources used by test. */
        if ((ret = MPI_Group_free(&world_group)))
            ERR(ret);

    } /* my_rank < TARGET_NTASKS */

    /* Finalize test. */
    printf("%d %s finalizing...\n", my_rank, TEST_NAME);
    if ((ret = pio_test_finalize(&test_comm)))
        return ERR_AWFUL;

    printf("%d %s SUCCESS!!\n", my_rank, TEST_NAME);

    return 0;
}
