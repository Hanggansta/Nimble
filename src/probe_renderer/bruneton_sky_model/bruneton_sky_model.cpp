#include "bruneton_sky_model.h"
#include "../../utility.h"
#include "../../logger.h"
#include "../../macros.h"
#include "../../resource_manager.h"
#include "../../renderer.h"

namespace nimble
{
static double kDefaultLambdas[]               = { kLambdaR, kLambdaG, kLambdaB };
static double kDefaultLuminanceFromRadiance[] = { 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0 };

// -----------------------------------------------------------------------------------------------------------------------------------

BrunetonSkyModel::BrunetonSkyModel()
{
}

// -----------------------------------------------------------------------------------------------------------------------------------

BrunetonSkyModel::~BrunetonSkyModel()
{
    for (auto layer : m_absorption_density)
    {
        NIMBLE_SAFE_DELETE(layer);
    }

    m_absorption_density.clear();

    NIMBLE_SAFE_DELETE(m_mie_density);
    NIMBLE_SAFE_DELETE(m_rayleigh_density);
    NIMBLE_SAFE_DELETE(m_texture_buffer);
}

// -----------------------------------------------------------------------------------------------------------------------------------

bool BrunetonSkyModel::initialize(int num_scattering_orders, Renderer* renderer, ResourceManager* res_mgr)
{
    std::vector<std::string> defines;

    if (m_use_luminance == LUMINANCE::NONE)
        defines.push_back("RADIANCE_API_ENABLED");

    if (m_combine_scattering_textures)
        defines.push_back("COMBINED_SCATTERING_TEXTURES");

    m_clear_2d_shader = res_mgr->load_shader("shader/bruneton_sky_model/clear_2d_cs.glsl", GL_COMPUTE_SHADER);

    if (m_clear_2d_shader)
    {
        m_clear_2d_program = renderer->create_program({ m_clear_2d_shader });

        if (!m_clear_2d_program)
        {
            NIMBLE_LOG_ERROR("Failed to create program");
            return false;
        }
    }
    else
    {
        NIMBLE_LOG_ERROR("Failed to load shaders");
        return false;
    }

    m_clear_3d_shader = res_mgr->load_shader("shader/bruneton_sky_model/clear_3d_cs.glsl", GL_COMPUTE_SHADER);

    if (m_clear_3d_shader)
    {
        m_clear_3d_program = renderer->create_program({ m_clear_3d_shader });

        if (!m_clear_3d_program)
        {
            NIMBLE_LOG_ERROR("Failed to create program");
            return false;
        }
    }
    else
    {
        NIMBLE_LOG_ERROR("Failed to load shaders");
        return false;
    }

    m_direct_irradiance_shader = res_mgr->load_shader("shader/bruneton_sky_model/compute_direct_irradiance_cs.glsl", GL_COMPUTE_SHADER, defines);

    if (m_direct_irradiance_shader)
    {
        m_direct_irradiance_program = renderer->create_program({ m_direct_irradiance_shader });

        if (!m_direct_irradiance_program)
        {
            NIMBLE_LOG_ERROR("Failed to create program");
            return false;
        }
    }
    else
    {
        NIMBLE_LOG_ERROR("Failed to load shaders");
        return false;
    }

    m_indirect_irradiance_shader = res_mgr->load_shader("shader/bruneton_sky_model/compute_indirect_irradiance_cs.glsl", GL_COMPUTE_SHADER, defines);

    if (m_indirect_irradiance_shader)
    {
        m_indirect_irradiance_program = renderer->create_program({ m_indirect_irradiance_shader });

        if (!m_indirect_irradiance_program)
        {
            NIMBLE_LOG_ERROR("Failed to create program");
            return false;
        }
    }
    else
    {
        NIMBLE_LOG_ERROR("Failed to load shaders");
        return false;
    }

    m_multiple_scattering_shader = res_mgr->load_shader("shader/bruneton_sky_model/compute_multiple_scattering_cs.glsl", GL_COMPUTE_SHADER, defines);

    if (m_multiple_scattering_shader)
    {
        m_multiple_scattering_program = renderer->create_program({ m_multiple_scattering_shader });

        if (!m_multiple_scattering_program)
        {
            NIMBLE_LOG_ERROR("Failed to create program");
            return false;
        }
    }
    else
    {
        NIMBLE_LOG_ERROR("Failed to load shaders");
        return false;
    }

    m_scattering_density_shader = res_mgr->load_shader("shader/bruneton_sky_model/compute_scattering_density_cs.glsl", GL_COMPUTE_SHADER, defines);

    if (m_scattering_density_shader)
    {
        m_scattering_density_program = renderer->create_program({ m_scattering_density_shader });

        if (!m_scattering_density_program)
        {
            NIMBLE_LOG_ERROR("Failed to create program");
            return false;
        }
    }
    else
    {
        NIMBLE_LOG_ERROR("Failed to load shaders");
        return false;
    }

    m_single_scattering_shader = res_mgr->load_shader("shader/bruneton_sky_model/compute_single_scattering_cs.glsl", GL_COMPUTE_SHADER, defines);

    if (m_single_scattering_shader)
    {
        m_single_scattering_program = renderer->create_program({ m_single_scattering_shader });

        if (!m_single_scattering_program)
        {
            NIMBLE_LOG_ERROR("Failed to create program");
            return false;
        }
    }
    else
    {
        NIMBLE_LOG_ERROR("Failed to load shaders");
        return false;
    }

    m_transmittance_shader = res_mgr->load_shader("shader/bruneton_sky_model/compute_transmittance_cs.glsl", GL_COMPUTE_SHADER, defines);

    if (m_transmittance_shader)
    {
        m_transmittance_program = renderer->create_program({ m_transmittance_shader });

        if (!m_transmittance_program)
        {
            NIMBLE_LOG_ERROR("Failed to create program");
            return false;
        }
    }
    else
    {
        NIMBLE_LOG_ERROR("Failed to load shaders");
        return false;
    }

    TextureBuffer* buffer = new TextureBuffer(m_half_precision);
    buffer->clear(m_clear_2d_program.get(), m_clear_3d_program.get());

    // The actual precomputations depend on whether we want to store precomputed
    // irradiance or illuminance values.
    if (num_precomputed_wavelengths() <= 3)
        precompute(buffer, nullptr, nullptr, false, num_scattering_orders);
    else
    {
        int    num_iterations = (num_precomputed_wavelengths() + 2) / 3;
        double dlambda        = (kLambdaMax - kLambdaMin) / (3.0 * num_iterations);

        for (int i = 0; i < num_iterations; ++i)
        {
            double lambdas[] = {
                kLambdaMin + (3 * i + 0.5) * dlambda,
                kLambdaMin + (3 * i + 1.5) * dlambda,
                kLambdaMin + (3 * i + 2.5) * dlambda
            };

            double luminance_from_radiance[] = {
                coeff(lambdas[0], 0) * dlambda, coeff(lambdas[1], 0) * dlambda, coeff(lambdas[2], 0) * dlambda, coeff(lambdas[0], 1) * dlambda, coeff(lambdas[1], 1) * dlambda, coeff(lambdas[2], 1) * dlambda, coeff(lambdas[0], 2) * dlambda, coeff(lambdas[1], 2) * dlambda, coeff(lambdas[2], 2) * dlambda
            };

            bool blend = i > 0;
            precompute(buffer, lambdas, luminance_from_radiance, blend, num_scattering_orders);
        }

        // After the above iterations, the transmittance texture contains the
        // transmittance for the 3 wavelengths used at the last iteration. But we
        // want the transmittance at kLambdaR, kLambdaG, kLambdaB instead, so we
        // must recompute it here for these 3 wavelengths:
        m_transmittance_program->use();

        bind_compute_uniforms(m_transmittance_program.get(), nullptr, nullptr);

        buffer->m_transmittance_array[WRITE]->bind_image(0, 0, 0, GL_READ_WRITE, buffer->m_transmittance_array[WRITE]->internal_format());

        m_transmittance_program->set_uniform("blend", glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));

        int NUM = CONSTANTS::NUM_THREADS;

        GL_CHECK_ERROR(glDispatchCompute(CONSTANTS::TRANSMITTANCE_WIDTH / NUM, CONSTANTS::TRANSMITTANCE_HEIGHT / NUM, 1));
        GL_CHECK_ERROR(glFinish());

        swap(buffer->m_transmittance_array);
    }

    //Grab ref to textures and mark as null in buffer so they are not released.
    m_transmittance_texture = buffer->m_transmittance_array[READ];
    m_scattering_texture    = buffer->m_scattering_array[READ];
    m_irradiance_texture    = buffer->m_irradiance_array[READ];

    if (m_combine_scattering_textures)
        m_optional_single_mie_scattering_texture = nullptr;
    else
        m_optional_single_mie_scattering_texture = buffer->m_optional_single_mie_scattering_array[READ];

    m_texture_buffer = buffer;

    return true;
}

// -----------------------------------------------------------------------------------------------------------------------------------

void BrunetonSkyModel::bind_rendering_uniforms(Program* program)
{
    if (program->set_uniform("transmittance_texture", 0))
        m_transmittance_texture->bind(0);

    if (program->set_uniform("scattering_texture", 1))
        m_scattering_texture->bind(1);

    if (program->set_uniform("irradiance_texture", 2))
        m_irradiance_texture->bind(2);

    if (!m_combine_scattering_textures)
    {
        if (program->set_uniform("single_mie_scattering_texture", 3))
            m_optional_single_mie_scattering_texture->bind(3);
    }

    program->set_uniform("TRANSMITTANCE_TEXTURE_WIDTH", CONSTANTS::TRANSMITTANCE_WIDTH);
    program->set_uniform("TRANSMITTANCE_TEXTURE_HEIGHT", CONSTANTS::TRANSMITTANCE_HEIGHT);
    program->set_uniform("SCATTERING_TEXTURE_R_SIZE", CONSTANTS::SCATTERING_R);
    program->set_uniform("SCATTERING_TEXTURE_MU_SIZE", CONSTANTS::SCATTERING_MU);
    program->set_uniform("SCATTERING_TEXTURE_MU_S_SIZE", CONSTANTS::SCATTERING_MU_S);
    program->set_uniform("SCATTERING_TEXTURE_NU_SIZE", CONSTANTS::SCATTERING_NU);
    program->set_uniform("SCATTERING_TEXTURE_WIDTH", CONSTANTS::SCATTERING_WIDTH);
    program->set_uniform("SCATTERING_TEXTURE_HEIGHT", CONSTANTS::SCATTERING_HEIGHT);
    program->set_uniform("SCATTERING_TEXTURE_DEPTH", CONSTANTS::SCATTERING_DEPTH);
    program->set_uniform("IRRADIANCE_TEXTURE_WIDTH", CONSTANTS::IRRADIANCE_WIDTH);
    program->set_uniform("IRRADIANCE_TEXTURE_HEIGHT", CONSTANTS::IRRADIANCE_HEIGHT);

    program->set_uniform("sun_angular_radius", (float)m_sun_angular_radius);
    program->set_uniform("bottom_radius", (float)(m_bottom_radius / m_length_unit_in_meters));
    program->set_uniform("top_radius", (float)(m_top_radius / m_length_unit_in_meters));
    program->set_uniform("mie_phase_function_g", (float)m_mie_phase_function_g);
    program->set_uniform("mu_s_min", (float)cos(m_max_sun_zenith_angle));

    glm::vec3 sky_spectral_radiance_to_luminance, sun_spectral_radiance_to_luminance;
    sky_sun_radiance_to_luminance(sky_spectral_radiance_to_luminance, sun_spectral_radiance_to_luminance);

    program->set_uniform("SKY_SPECTRAL_RADIANCE_TO_LUMINANCE", sky_spectral_radiance_to_luminance);
    program->set_uniform("SUN_SPECTRAL_RADIANCE_TO_LUMINANCE", sun_spectral_radiance_to_luminance);

    double lambdas[] = { kLambdaR, kLambdaG, kLambdaB };

    glm::vec3 solar_irradiance = to_vector(m_wave_lengths, m_solar_irradiance, lambdas, 1.0);
    program->set_uniform("solar_irradiance", solar_irradiance);

    glm::vec3 rayleigh_scattering = to_vector(m_wave_lengths, m_rayleigh_scattering, lambdas, m_length_unit_in_meters);
    program->set_uniform("rayleigh_scattering", rayleigh_scattering);

    glm::vec3 mie_scattering = to_vector(m_wave_lengths, m_mie_scattering, lambdas, m_length_unit_in_meters);
    program->set_uniform("mie_scattering", mie_scattering);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void BrunetonSkyModel::convert_spectrum_to_linear_srgb(double& r, double& g, double& b)
{
    double    x       = 0.0;
    double    y       = 0.0;
    double    z       = 0.0;
    const int dlambda = 1;
    for (int lambda = kLambdaMin; lambda < kLambdaMax; lambda += dlambda)
    {
        double value = interpolate(m_wave_lengths, m_solar_irradiance, lambda);
        x += cie_color_matching_function_table_value(lambda, 1) * value;
        y += cie_color_matching_function_table_value(lambda, 2) * value;
        z += cie_color_matching_function_table_value(lambda, 3) * value;
    }

    const double* XYZ_TO_SRGB = &kXYZ_TO_SRGB[0];
    r                         = static_cast<double>(CONSTANTS::MAX_LUMINOUS_EFFICACY) * (XYZ_TO_SRGB[0] * x + XYZ_TO_SRGB[1] * y + XYZ_TO_SRGB[2] * z) * dlambda;
    g                         = static_cast<double>(CONSTANTS::MAX_LUMINOUS_EFFICACY) * (XYZ_TO_SRGB[3] * x + XYZ_TO_SRGB[4] * y + XYZ_TO_SRGB[5] * z) * dlambda;
    b                         = static_cast<double>(CONSTANTS::MAX_LUMINOUS_EFFICACY) * (XYZ_TO_SRGB[6] * x + XYZ_TO_SRGB[7] * y + XYZ_TO_SRGB[8] * z) * dlambda;
}

// -----------------------------------------------------------------------------------------------------------------------------------

double BrunetonSkyModel::coeff(double lambda, int component)
{
    // Note that we don't include MAX_LUMINOUS_EFFICACY here, to avoid
    // artefacts due to too large values when using half precision on GPU.
    // We add this term back in kAtmosphereShader, via
    // SKY_SPECTRAL_RADIANCE_TO_LUMINANCE (see also the comments in the
    // Model constructor).
    double x    = cie_color_matching_function_table_value(lambda, 1);
    double y    = cie_color_matching_function_table_value(lambda, 2);
    double z    = cie_color_matching_function_table_value(lambda, 3);
    double sRGB = kXYZ_TO_SRGB[component * 3 + 0] * x + kXYZ_TO_SRGB[component * 3 + 1] * y + kXYZ_TO_SRGB[component * 3 + 2] * z;

    return sRGB;
}

// -----------------------------------------------------------------------------------------------------------------------------------

void BrunetonSkyModel::bind_compute_uniforms(Program* program, double* lambdas, double* luminance_from_radiance)
{
    if (lambdas == nullptr)
        lambdas = kDefaultLambdas;

    if (luminance_from_radiance == nullptr)
        luminance_from_radiance = kDefaultLuminanceFromRadiance;

    program->set_uniform("TRANSMITTANCE_TEXTURE_WIDTH", CONSTANTS::TRANSMITTANCE_WIDTH);
    program->set_uniform("TRANSMITTANCE_TEXTURE_HEIGHT", CONSTANTS::TRANSMITTANCE_HEIGHT);
    program->set_uniform("SCATTERING_TEXTURE_R_SIZE", CONSTANTS::SCATTERING_R);
    program->set_uniform("SCATTERING_TEXTURE_MU_SIZE", CONSTANTS::SCATTERING_MU);
    program->set_uniform("SCATTERING_TEXTURE_MU_S_SIZE", CONSTANTS::SCATTERING_MU_S);
    program->set_uniform("SCATTERING_TEXTURE_NU_SIZE", CONSTANTS::SCATTERING_NU);
    program->set_uniform("SCATTERING_TEXTURE_WIDTH", CONSTANTS::SCATTERING_WIDTH);
    program->set_uniform("SCATTERING_TEXTURE_HEIGHT", CONSTANTS::SCATTERING_HEIGHT);
    program->set_uniform("SCATTERING_TEXTURE_DEPTH", CONSTANTS::SCATTERING_DEPTH);
    program->set_uniform("IRRADIANCE_TEXTURE_WIDTH", CONSTANTS::IRRADIANCE_WIDTH);
    program->set_uniform("IRRADIANCE_TEXTURE_HEIGHT", CONSTANTS::IRRADIANCE_HEIGHT);

    glm::vec3 sky_spectral_radiance_to_luminance, sun_spectral_radiance_to_luminance;
    sky_sun_radiance_to_luminance(sky_spectral_radiance_to_luminance, sun_spectral_radiance_to_luminance);

    program->set_uniform("SKY_SPECTRAL_RADIANCE_TO_LUMINANCE", sky_spectral_radiance_to_luminance);
    program->set_uniform("SUN_SPECTRAL_RADIANCE_TO_LUMINANCE", sun_spectral_radiance_to_luminance);

    glm::vec3 solar_irradiance = to_vector(m_wave_lengths, m_solar_irradiance, lambdas, 1.0);
    program->set_uniform("solar_irradiance", solar_irradiance);

    glm::vec3 rayleigh_scattering = to_vector(m_wave_lengths, m_rayleigh_scattering, lambdas, m_length_unit_in_meters);
    bind_density_layer(program, m_rayleigh_density);
    program->set_uniform("rayleigh_scattering", rayleigh_scattering);

    glm::vec3 mie_scattering = to_vector(m_wave_lengths, m_mie_scattering, lambdas, m_length_unit_in_meters);
    glm::vec3 mie_extinction = to_vector(m_wave_lengths, m_mie_extinction, lambdas, m_length_unit_in_meters);
    bind_density_layer(program, m_mie_density);
    program->set_uniform("mie_scattering", mie_scattering);
    program->set_uniform("mie_extinction", mie_extinction);

    glm::vec3 absorption_extinction = to_vector(m_wave_lengths, m_absorption_extinction, lambdas, m_length_unit_in_meters);
    bind_density_layer(program, m_absorption_density[0]);
    bind_density_layer(program, m_absorption_density[1]);
    program->set_uniform("absorption_extinction", absorption_extinction);

    glm::vec3 groundAlbedo = to_vector(m_wave_lengths, m_ground_albedo, lambdas, 1.0);
    program->set_uniform("ground_albedo", groundAlbedo);

    program->set_uniform("luminanceFromRadiance", to_matrix(luminance_from_radiance));
    program->set_uniform("sun_angular_radius", (float)m_sun_angular_radius);
    program->set_uniform("bottom_radius", (float)(m_bottom_radius / m_length_unit_in_meters));
    program->set_uniform("top_radius", (float)(m_top_radius / m_length_unit_in_meters));
    program->set_uniform("mie_phase_function_g", (float)m_mie_phase_function_g);
    program->set_uniform("mu_s_min", (float)cos(m_max_sun_zenith_angle));
}

// -----------------------------------------------------------------------------------------------------------------------------------

void BrunetonSkyModel::bind_density_layer(Program* program, DensityProfileLayer* layer)
{
    program->set_uniform(layer->name + "_width", (float)(layer->width / m_length_unit_in_meters));
    program->set_uniform(layer->name + "_exp_term", (float)layer->exp_term);
    program->set_uniform(layer->name + "_exp_scale", (float)(layer->exp_scale * m_length_unit_in_meters));
    program->set_uniform(layer->name + "_linear_term", (float)(layer->linear_term * m_length_unit_in_meters));
    program->set_uniform(layer->name + "_constant_term", (float)layer->constant_term);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void BrunetonSkyModel::sky_sun_radiance_to_luminance(glm::vec3& sky_spectral_radiance_to_luminance, glm::vec3& sun_spectral_radiance_to_luminance)
{
    bool   precompute_illuminance = num_precomputed_wavelengths() > 3;
    double sky_k_r, sky_k_g, sky_k_b;

    if (precompute_illuminance)
        sky_k_r = sky_k_g = sky_k_b = static_cast<double>(CONSTANTS::MAX_LUMINOUS_EFFICACY);
    else
        compute_spectral_radiance_to_luminance_factors(m_wave_lengths, m_solar_irradiance, -3, sky_k_r, sky_k_g, sky_k_b);

    // Compute the values for the SUN_RADIANCE_TO_LUMINANCE constant.
    double sun_k_r, sun_k_g, sun_k_b;
    compute_spectral_radiance_to_luminance_factors(m_wave_lengths, m_solar_irradiance, 0, sun_k_r, sun_k_g, sun_k_b);

    sky_spectral_radiance_to_luminance = glm::vec3((float)sky_k_r, (float)sky_k_g, (float)sky_k_b);
    sun_spectral_radiance_to_luminance = glm::vec3((float)sun_k_r, (float)sun_k_g, (float)sun_k_b);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void BrunetonSkyModel::precompute(TextureBuffer* buffer, double* lambdas, double* luminance_from_radiance, bool blend, int num_scattering_orders)
{
    int BLEND       = blend ? 1 : 0;
    int NUM_THREADS = CONSTANTS::NUM_THREADS;

    // ------------------------------------------------------------------
    // Compute Transmittance
    // ------------------------------------------------------------------

    m_transmittance_program->use();

    bind_compute_uniforms(m_transmittance_program.get(), lambdas, luminance_from_radiance);

    // Compute the transmittance, and store it in transmittance_texture
    buffer->m_transmittance_array[WRITE]->bind_image(0, 0, 0, GL_READ_WRITE, buffer->m_transmittance_array[WRITE]->internal_format());

    m_transmittance_program->set_uniform("blend", glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));

    GL_CHECK_ERROR(glDispatchCompute(CONSTANTS::TRANSMITTANCE_WIDTH / NUM_THREADS, CONSTANTS::TRANSMITTANCE_HEIGHT / NUM_THREADS, 1));
    GL_CHECK_ERROR(glFinish());

    swap(buffer->m_transmittance_array);

    // ------------------------------------------------------------------
    // Compute Direct Irradiance
    // ------------------------------------------------------------------

    m_direct_irradiance_program->use();

    bind_compute_uniforms(m_direct_irradiance_program.get(), lambdas, luminance_from_radiance);

    // Compute the direct irradiance, store it in delta_irradiance_texture and,
    // depending on 'blend', either initialize irradiance_texture_ with zeros or
    // leave it unchanged (we don't want the direct irradiance in
    // irradiance_texture_, but only the irradiance from the sky).
    buffer->m_irradiance_array[READ]->bind_image(0, 0, 0, GL_READ_WRITE, buffer->m_irradiance_array[READ]->internal_format());
    buffer->m_irradiance_array[WRITE]->bind_image(1, 0, 0, GL_READ_WRITE, buffer->m_irradiance_array[WRITE]->internal_format());
    buffer->m_delta_irradiance_texture->bind_image(2, 0, 0, GL_READ_WRITE, buffer->m_delta_irradiance_texture->internal_format());

    if (m_direct_irradiance_program->set_uniform("transmittance", 3))
        buffer->m_transmittance_array[READ]->bind(3);

    m_direct_irradiance_program->set_uniform("blend", glm::vec4(0.0f, BLEND, 0.0f, 0.0f));

    GL_CHECK_ERROR(glDispatchCompute(CONSTANTS::IRRADIANCE_WIDTH / NUM_THREADS, CONSTANTS::IRRADIANCE_HEIGHT / NUM_THREADS, 1));
    GL_CHECK_ERROR(glFinish());

    swap(buffer->m_irradiance_array);

    // ------------------------------------------------------------------
    // Compute Single Scattering
    // ------------------------------------------------------------------

    m_single_scattering_program->use();

    bind_compute_uniforms(m_single_scattering_program.get(), lambdas, luminance_from_radiance);

    // Compute the rayleigh and mie single scattering, store them in
    // delta_rayleigh_scattering_texture and delta_mie_scattering_texture, and
    // either store them or accumulate them in scattering_texture_ and
    // optional_single_mie_scattering_texture_.
    buffer->m_delta_rayleigh_scattering_texture->bind_image(0, 0, 0, GL_READ_WRITE, buffer->m_delta_rayleigh_scattering_texture->internal_format());
    buffer->m_delta_mie_scattering_texture->bind_image(1, 0, 0, GL_READ_WRITE, buffer->m_delta_mie_scattering_texture->internal_format());
    buffer->m_scattering_array[READ]->bind_image(2, 0, 0, GL_READ_WRITE, buffer->m_scattering_array[READ]->internal_format());
    buffer->m_scattering_array[WRITE]->bind_image(3, 0, 0, GL_READ_WRITE, buffer->m_scattering_array[WRITE]->internal_format());
    buffer->m_optional_single_mie_scattering_array[READ]->bind_image(4, 0, 0, GL_READ_WRITE, buffer->m_optional_single_mie_scattering_array[READ]->internal_format());
    buffer->m_optional_single_mie_scattering_array[WRITE]->bind_image(5, 0, 0, GL_READ_WRITE, buffer->m_optional_single_mie_scattering_array[WRITE]->internal_format());

    if (m_single_scattering_program->set_uniform("transmittance", 6))
        buffer->m_transmittance_array[READ]->bind(6);

    m_single_scattering_program->set_uniform("blend", glm::vec4(0.0f, 0.0f, BLEND, BLEND));

    for (int layer = 0; layer < CONSTANTS::SCATTERING_DEPTH; ++layer)
    {
        m_single_scattering_program->set_uniform("layer", layer);

        GL_CHECK_ERROR(glDispatchCompute(CONSTANTS::SCATTERING_WIDTH / NUM_THREADS, CONSTANTS::SCATTERING_HEIGHT / NUM_THREADS, 1));
        GL_CHECK_ERROR(glFinish());
    }

    swap(buffer->m_scattering_array);
    swap(buffer->m_optional_single_mie_scattering_array);

    // Compute the 2nd, 3rd and 4th order of scattering, in sequence.
    for (int scattering_order = 2; scattering_order <= num_scattering_orders; ++scattering_order)
    {
        // ------------------------------------------------------------------
        // Compute Scattering Density
        // ------------------------------------------------------------------

        m_scattering_density_program->use();

        bind_compute_uniforms(m_scattering_density_program.get(), lambdas, luminance_from_radiance);

        // Compute the scattering density, and store it in
        // delta_scattering_density_texture.
        buffer->m_delta_scattering_density_texture->bind_image(0, 0, 0, GL_READ_WRITE, buffer->m_delta_scattering_density_texture->internal_format());

        if (m_scattering_density_program->set_uniform("transmittance", 1))
            buffer->m_transmittance_array[READ]->bind(1);

        if (m_scattering_density_program->set_uniform("single_rayleigh_scattering", 2))
            buffer->m_delta_rayleigh_scattering_texture->bind(2);

        if (m_scattering_density_program->set_uniform("single_mie_scattering", 3))
            buffer->m_delta_mie_scattering_texture->bind(3);

        if (m_scattering_density_program->set_uniform("multiple_scattering", 4))
            buffer->m_delta_multiple_scattering_texture->bind(4);

        if (m_scattering_density_program->set_uniform("irradiance", 5))
            buffer->m_delta_irradiance_texture->bind(5);

        m_scattering_density_program->set_uniform("scattering_order", scattering_order);
        m_scattering_density_program->set_uniform("blend", glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));

        for (int layer = 0; layer < CONSTANTS::SCATTERING_DEPTH; ++layer)
        {
            m_scattering_density_program->set_uniform("layer", layer);
            GL_CHECK_ERROR(glDispatchCompute(CONSTANTS::SCATTERING_WIDTH / NUM_THREADS, CONSTANTS::SCATTERING_HEIGHT / NUM_THREADS, 1));
            GL_CHECK_ERROR(glFinish());
        }

        // ------------------------------------------------------------------
        // Compute Indirect Irradiance
        // ------------------------------------------------------------------

        m_indirect_irradiance_program->use();

        bind_compute_uniforms(m_indirect_irradiance_program.get(), lambdas, luminance_from_radiance);

        // Compute the indirect irradiance, store it in delta_irradiance_texture and
        // accumulate it in irradiance_texture_.
        buffer->m_delta_irradiance_texture->bind_image(0, 0, 0, GL_READ_WRITE, buffer->m_delta_irradiance_texture->internal_format());
        buffer->m_irradiance_array[READ]->bind_image(1, 0, 0, GL_READ_WRITE, buffer->m_irradiance_array[READ]->internal_format());
        buffer->m_irradiance_array[WRITE]->bind_image(2, 0, 0, GL_READ_WRITE, buffer->m_irradiance_array[WRITE]->internal_format());

        if (m_indirect_irradiance_program->set_uniform("single_rayleigh_scattering", 3))
            buffer->m_delta_rayleigh_scattering_texture->bind(3);

        if (m_indirect_irradiance_program->set_uniform("single_mie_scattering", 4))
            buffer->m_delta_mie_scattering_texture->bind(4);

        if (m_indirect_irradiance_program->set_uniform("multiple_scattering", 5))
            buffer->m_delta_multiple_scattering_texture->bind(5);

        m_indirect_irradiance_program->set_uniform("scattering_order", scattering_order - 1);
        m_indirect_irradiance_program->set_uniform("blend", glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));

        GL_CHECK_ERROR(glDispatchCompute(CONSTANTS::IRRADIANCE_WIDTH / NUM_THREADS, CONSTANTS::IRRADIANCE_HEIGHT / NUM_THREADS, 1));
        GL_CHECK_ERROR(glFinish());

        swap(buffer->m_irradiance_array);

        // ------------------------------------------------------------------
        // Compute Multiple Scattering
        // ------------------------------------------------------------------

        m_multiple_scattering_program->use();

        bind_compute_uniforms(m_multiple_scattering_program.get(), lambdas, luminance_from_radiance);

        // Compute the multiple scattering, store it in
        // delta_multiple_scattering_texture, and accumulate it in
        // scattering_texture_.
        buffer->m_delta_multiple_scattering_texture->bind_image(0, 0, 0, GL_READ_WRITE, buffer->m_delta_multiple_scattering_texture->internal_format());
        buffer->m_scattering_array[READ]->bind_image(1, 0, 0, GL_READ_WRITE, buffer->m_scattering_array[READ]->internal_format());
        buffer->m_scattering_array[WRITE]->bind_image(2, 0, 0, GL_READ_WRITE, buffer->m_scattering_array[WRITE]->internal_format());

        if (m_multiple_scattering_program->set_uniform("transmittance", 3))
            buffer->m_transmittance_array[READ]->bind(3);

        if (m_multiple_scattering_program->set_uniform("delta_scattering_density", 4))
            buffer->m_delta_scattering_density_texture->bind(4);

        m_multiple_scattering_program->set_uniform("blend", glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));

        for (int layer = 0; layer < CONSTANTS::SCATTERING_DEPTH; ++layer)
        {
            m_multiple_scattering_program->set_uniform("layer", layer);
            GL_CHECK_ERROR(glDispatchCompute(CONSTANTS::SCATTERING_WIDTH / NUM_THREADS, CONSTANTS::SCATTERING_HEIGHT / NUM_THREADS, 1));
            GL_CHECK_ERROR(glFinish());
        }

        swap(buffer->m_scattering_array);
    }
}

// -----------------------------------------------------------------------------------------------------------------------------------

void BrunetonSkyModel::swap(Texture** arr)
{
    Texture* tmp = arr[READ];
    arr[READ]    = arr[WRITE];
    arr[WRITE]   = tmp;
}

// -----------------------------------------------------------------------------------------------------------------------------------

glm::vec3 BrunetonSkyModel::to_vector(const std::vector<double>& wavelengths, const std::vector<double>& v, double lambdas[], double scale)
{
    double r = interpolate(wavelengths, v, lambdas[0]) * scale;
    double g = interpolate(wavelengths, v, lambdas[1]) * scale;
    double b = interpolate(wavelengths, v, lambdas[2]) * scale;

    return glm::vec3((float)r, (float)g, (float)b);
}

// -----------------------------------------------------------------------------------------------------------------------------------

glm::mat4 BrunetonSkyModel::to_matrix(double arr[])
{
    return glm::mat4(
        (float)arr[0], (float)arr[3], (float)arr[6], 0.0f, (float)arr[1], (float)arr[4], (float)arr[7], 0.0f, (float)arr[2], (float)arr[5], (float)arr[8], 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
}

// -----------------------------------------------------------------------------------------------------------------------------------

double BrunetonSkyModel::cie_color_matching_function_table_value(double wavelength, int column)
{
    if (wavelength <= kLambdaMin || wavelength >= kLambdaMax)
        return 0.0;

    double u   = (wavelength - kLambdaMin) / 5.0;
    int    row = (int)floor(u);

    u -= row;
    return kCIE_2_DEG_COLOR_MATCHING_FUNCTIONS[4 * row + column] * (1.0 - u) + kCIE_2_DEG_COLOR_MATCHING_FUNCTIONS[4 * (row + 1) + column] * u;
}

// -----------------------------------------------------------------------------------------------------------------------------------

double BrunetonSkyModel::interpolate(const std::vector<double>& wavelengths, const std::vector<double>& wavelength_function, double wavelength)
{
    if (wavelength < wavelengths[0])
        return wavelength_function[0];

    for (int i = 0; i < wavelengths.size() - 1; ++i)
    {
        if (wavelength < wavelengths[i + 1])
        {
            double u = (wavelength - wavelengths[i]) / (wavelengths[i + 1] - wavelengths[i]);
            return wavelength_function[i] * (1.0 - u) + wavelength_function[i + 1] * u;
        }
    }

    return wavelength_function[wavelength_function.size() - 1];
}

// -----------------------------------------------------------------------------------------------------------------------------------

void BrunetonSkyModel::compute_spectral_radiance_to_luminance_factors(const std::vector<double>& wavelengths, const std::vector<double>& solar_irradiance, double lambda_power, double& k_r, double& k_g, double& k_b)
{
    k_r            = 0.0;
    k_g            = 0.0;
    k_b            = 0.0;
    double solar_r = interpolate(wavelengths, solar_irradiance, kLambdaR);
    double solar_g = interpolate(wavelengths, solar_irradiance, kLambdaG);
    double solar_b = interpolate(wavelengths, solar_irradiance, kLambdaB);
    int    dlambda = 1;

    for (int lambda = kLambdaMin; lambda < kLambdaMax; lambda += dlambda)
    {
        double x_bar = cie_color_matching_function_table_value(lambda, 1);
        double y_bar = cie_color_matching_function_table_value(lambda, 2);
        double z_bar = cie_color_matching_function_table_value(lambda, 3);

        const double* xyz2srgb   = &kXYZ_TO_SRGB[0];
        double        r_bar      = xyz2srgb[0] * x_bar + xyz2srgb[1] * y_bar + xyz2srgb[2] * z_bar;
        double        g_bar      = xyz2srgb[3] * x_bar + xyz2srgb[4] * y_bar + xyz2srgb[5] * z_bar;
        double        b_bar      = xyz2srgb[6] * x_bar + xyz2srgb[7] * y_bar + xyz2srgb[8] * z_bar;
        double        irradiance = interpolate(wavelengths, solar_irradiance, lambda);

        k_r += r_bar * irradiance / solar_r * pow(lambda / kLambdaR, lambda_power);
        k_g += g_bar * irradiance / solar_g * pow(lambda / kLambdaG, lambda_power);
        k_b += b_bar * irradiance / solar_b * pow(lambda / kLambdaB, lambda_power);
    }

    k_r *= static_cast<double>(CONSTANTS::MAX_LUMINOUS_EFFICACY) * dlambda;
    k_g *= static_cast<double>(CONSTANTS::MAX_LUMINOUS_EFFICACY) * dlambda;
    k_b *= static_cast<double>(CONSTANTS::MAX_LUMINOUS_EFFICACY) * dlambda;
}

// -----------------------------------------------------------------------------------------------------------------------------------

} // namespace nimble