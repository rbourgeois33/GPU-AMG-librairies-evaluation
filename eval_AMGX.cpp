#include <iostream>
#include <string>
#include <amgx_c.h>

void check_amgx_error(AMGX_RC rc, const char *msg) {
    if (rc != AMGX_RC_OK) {
        char err_string[256];
        AMGX_get_error_string(rc, err_string, sizeof(err_string));
        std::cerr << "Error: " << msg << " - " << err_string << std::endl;
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char* argv[])
{
    // ---------------------------------------------------------------------
    // 1. Parse input arguments
    // ---------------------------------------------------------------------

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <config_file.json>" << std::endl;
        return EXIT_FAILURE;
    }

    //This file has to be generated using ../data/AMGX_formatter.sh to contain both A and rhs
    std::string system_file = "../data/AMGX_system.mtx";

    //RC object for error handling
    AMGX_RC rc;

    // ---------------------------------------------------------------------
    // 2. Initialize AMGX
    // ---------------------------------------------------------------------
    rc = AMGX_initialize();
    check_amgx_error(rc, "AMGX_initialize error:");

    // If you plan to use MPI, call AMGX_initialize_plugins(); for single GPU, it's usually not needed.

    // Optionally, capture and print AMGX version:
    int major, minor, patch;
    AMGX_get_api_version(&major, &minor);
    std::cout << "Using AMGX API version: " << major << "." << minor << "." << patch << std::endl;

    // ---------------------------------------------------------------------
    // 3. Create AMGX configuration from file
    // --------------------------------------------------------------------
    // Load configuration from JSON file
    AMGX_config_handle config = nullptr;
    const char* config_file = argv[1];
    rc = AMGX_config_create_from_file(&config, config_file);
    check_amgx_error(rc, "AMGX_config_create error:");


    // ---------------------------------------------------------------------
    // 4. Create AMGX resources
    // ---------------------------------------------------------------------
    // For single GPU, we pass null communicator (no MPI) and set device to 0.
    AMGX_resources_handle rsrc = NULL;
    int device_id = 0;
    AMGX_resources_create_simple(&rsrc, config);

    // ---------------------------------------------------------------------
    // 5. Create solver object
    // ---------------------------------------------------------------------
    AMGX_solver_handle solver = NULL;
    rc = AMGX_solver_create(&solver, rsrc, AMGX_mode_dDDI, config);
    check_amgx_error(rc, "AMGX_solver_create:");

    // ---------------------------------------------------------------------
    // 6. Create matrix and vectors
    // ---------------------------------------------------------------------
    AMGX_matrix_handle A = NULL;
    AMGX_vector_handle x = NULL;
    AMGX_vector_handle b = NULL;

    AMGX_matrix_create(&A, rsrc, AMGX_mode_dDDI);
    AMGX_vector_create(&x, rsrc, AMGX_mode_dDDI);
    AMGX_vector_create(&b, rsrc, AMGX_mode_dDDI);

    // ---------------------------------------------------------------------
    // 7. Read system from .mtx file
    // ---------------------------------------------------------------------    
    rc = AMGX_read_system(A, b, x, system_file.c_str());
    check_amgx_error(rc, "AMGX_read_system:");

    // ---------------------------------------------------------------------
    // 8. Setup the solver (analysis phase)
    // ---------------------------------------------------------------------
    rc = AMGX_solver_setup(solver, A);
    check_amgx_error(rc, "AMGX_solver_setup:");
 
    // ---------------------------------------------------------------------
    // 9. Solve the system
    // ---------------------------------------------------------------------
    rc = AMGX_solver_solve(solver, b, x);
    check_amgx_error(rc, "AMGX_solver_setup:");

    // Optionally retrieve iteration count or final residual:
    int iters;
    AMGX_solver_get_iterations_number(solver, &iters);
    std::cout << "Number of iterations: " << iters << std::endl;

    // ---------------------------------------------------------------------
    // 10. Clean up and shut down
    // ---------------------------------------------------------------------
    AMGX_solver_destroy(solver);
    AMGX_matrix_destroy(A);
    AMGX_vector_destroy(x);
    AMGX_vector_destroy(b);
    AMGX_resources_destroy(rsrc);
    AMGX_config_destroy(config);
    
    // Finalize AMGX
    AMGX_finalize();
    std::cout << "AMGX solve complete." << std::endl;

    return EXIT_SUCCESS;
}