#include "Programs/ProgramOpenGL.hpp"
#include <OpenGL/Programs/CreateProgram.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>
#include <utility>

namespace {

using ::testing::InSequence;
using ::testing::Return;
using ::testing::StrEq;
using ::testing::StrictMock;

constexpr const char* vertexShaderSource = "vertex shader";
constexpr const char* fragmentShaderSource = "fragment shader";
constexpr GLuint vertexShaderId = 11U;
constexpr GLuint fragmentShaderId = 22U;
constexpr GLuint programId = 33U;

class MockProgramOpenGL {
  public:
    MOCK_METHOD(GLuint, create_shader, (GLenum shaderType));
    MOCK_METHOD(void, set_shader_source, (GLuint shader, const char* shaderSource));
    MOCK_METHOD(void, compile_shader, (GLuint shader));
    MOCK_METHOD(bool, get_shader_compile_status, (GLuint shader));
    MOCK_METHOD(std::string, get_shader_info_log, (GLuint shader));
    MOCK_METHOD(void, delete_shader, (GLuint shader));

    MOCK_METHOD(GLuint, create_program, ());
    MOCK_METHOD(void, attach_shader, (GLuint program, GLuint shader));
    MOCK_METHOD(void, link_program, (GLuint program));
    MOCK_METHOD(bool, get_program_link_status, (GLuint program));
    MOCK_METHOD(std::string, get_program_info_log, (GLuint program));
    MOCK_METHOD(void, delete_program, (GLuint program));
};

MockProgramOpenGL* activeMock{nullptr};

class CreateProgramTest : public ::testing::Test {
  protected:
    void SetUp() override { activeMock = &mock; }

    void TearDown() override { activeMock = nullptr; }

    StrictMock<MockProgramOpenGL> mock;
};

void expect_error_code(const opengl::ProgramCreationResult& result, opengl::ProgramCreationFailureStage expectedStage) {
    ASSERT_FALSE(result.has_value());
    const std::error_code expectedError = opengl::make_error_code(expectedStage);
    EXPECT_TRUE(result.error());
    EXPECT_EQ(expectedError, result.error());
    EXPECT_EQ(expectedError.value(), result.error().value());
    EXPECT_EQ(&opengl::program_creation_error_category(), &result.error().category());
    EXPECT_STREQ("opengl.program_creation", result.error().category().name());
    EXPECT_EQ(expectedError.message(), result.error().message());
}

void expect_successful_vertex_shader_setup(MockProgramOpenGL& mock) {
    EXPECT_CALL(mock, create_shader(GL_VERTEX_SHADER)).WillOnce(Return(vertexShaderId));
    EXPECT_CALL(mock, set_shader_source(vertexShaderId, StrEq(vertexShaderSource)));
    EXPECT_CALL(mock, compile_shader(vertexShaderId));
    EXPECT_CALL(mock, get_shader_compile_status(vertexShaderId)).WillOnce(Return(true));
}

void expect_successful_fragment_shader_setup(MockProgramOpenGL& mock) {
    EXPECT_CALL(mock, create_shader(GL_FRAGMENT_SHADER)).WillOnce(Return(fragmentShaderId));
    EXPECT_CALL(mock, set_shader_source(fragmentShaderId, StrEq(fragmentShaderSource)));
    EXPECT_CALL(mock, compile_shader(fragmentShaderId));
    EXPECT_CALL(mock, get_shader_compile_status(fragmentShaderId)).WillOnce(Return(true));
}

} // namespace

namespace opengl::program_opengl {

GLuint create_shader(GLenum shaderType) {
    return activeMock->create_shader(shaderType);
}

void set_shader_source(GLuint shader, const char* shaderSource) {
    activeMock->set_shader_source(shader, shaderSource);
}

void compile_shader(GLuint shader) {
    activeMock->compile_shader(shader);
}

bool get_shader_compile_status(GLuint shader) {
    return activeMock->get_shader_compile_status(shader);
}

std::string get_shader_info_log(GLuint shader) {
    return activeMock->get_shader_info_log(shader);
}

void delete_shader(GLuint shader) {
    activeMock->delete_shader(shader);
}

GLuint create_program() {
    return activeMock->create_program();
}

void attach_shader(GLuint program, GLuint shader) {
    activeMock->attach_shader(program, shader);
}

void link_program(GLuint program) {
    activeMock->link_program(program);
}

bool get_program_link_status(GLuint program) {
    return activeMock->get_program_link_status(program);
}

std::string get_program_info_log(GLuint program) {
    return activeMock->get_program_info_log(program);
}

void delete_program(GLuint program) {
    activeMock->delete_program(program);
}

} // namespace opengl::program_opengl

TEST_F(CreateProgramTest, ReturnsProgramHandleWhenShadersCompileAndProgramLinks) {
    {
        InSequence sequence;
        expect_successful_vertex_shader_setup(mock);
        expect_successful_fragment_shader_setup(mock);
        EXPECT_CALL(mock, create_program()).WillOnce(Return(programId));
        EXPECT_CALL(mock, attach_shader(programId, vertexShaderId));
        EXPECT_CALL(mock, attach_shader(programId, fragmentShaderId));
        EXPECT_CALL(mock, link_program(programId));
        EXPECT_CALL(mock, get_program_link_status(programId)).WillOnce(Return(true));
        EXPECT_CALL(mock, delete_shader(fragmentShaderId));
        EXPECT_CALL(mock, delete_shader(vertexShaderId));
    }

    opengl::ProgramCreationResult result = opengl::create_program(vertexShaderSource, fragmentShaderSource);

    ASSERT_TRUE(result.has_value());
    opengl::ProgramHandle program = std::move(result).value();
    EXPECT_EQ(programId, program.get_value());

    EXPECT_CALL(mock, delete_program(programId));
    program.reset();
}

TEST_F(CreateProgramTest, ReportsVertexShaderCreationFailure) {
    EXPECT_CALL(mock, create_shader(GL_VERTEX_SHADER)).WillOnce(Return(0U));

    const opengl::ProgramCreationResult result = opengl::create_program(vertexShaderSource, fragmentShaderSource);

    expect_error_code(result, opengl::ProgramCreationFailureStage::VertexShaderCreation);
}

TEST_F(CreateProgramTest, ReportsVertexShaderCompilationFailure) {
    {
        InSequence sequence;
        EXPECT_CALL(mock, create_shader(GL_VERTEX_SHADER)).WillOnce(Return(vertexShaderId));
        EXPECT_CALL(mock, set_shader_source(vertexShaderId, StrEq(vertexShaderSource)));
        EXPECT_CALL(mock, compile_shader(vertexShaderId));
        EXPECT_CALL(mock, get_shader_compile_status(vertexShaderId)).WillOnce(Return(false));
        EXPECT_CALL(mock, get_shader_info_log(vertexShaderId)).WillOnce(Return("vertex compile error"));
        EXPECT_CALL(mock, delete_shader(vertexShaderId));
    }

    const opengl::ProgramCreationResult result = opengl::create_program(vertexShaderSource, fragmentShaderSource);

    expect_error_code(result, opengl::ProgramCreationFailureStage::VertexShaderCompilation);
}

TEST_F(CreateProgramTest, ReportsFragmentShaderCreationFailure) {
    {
        InSequence sequence;
        expect_successful_vertex_shader_setup(mock);
        EXPECT_CALL(mock, create_shader(GL_FRAGMENT_SHADER)).WillOnce(Return(0U));
        EXPECT_CALL(mock, delete_shader(vertexShaderId));
    }

    const opengl::ProgramCreationResult result = opengl::create_program(vertexShaderSource, fragmentShaderSource);

    expect_error_code(result, opengl::ProgramCreationFailureStage::FragmentShaderCreation);
}

TEST_F(CreateProgramTest, ReportsFragmentShaderCompilationFailure) {
    {
        InSequence sequence;
        expect_successful_vertex_shader_setup(mock);
        EXPECT_CALL(mock, create_shader(GL_FRAGMENT_SHADER)).WillOnce(Return(fragmentShaderId));
        EXPECT_CALL(mock, set_shader_source(fragmentShaderId, StrEq(fragmentShaderSource)));
        EXPECT_CALL(mock, compile_shader(fragmentShaderId));
        EXPECT_CALL(mock, get_shader_compile_status(fragmentShaderId)).WillOnce(Return(false));
        EXPECT_CALL(mock, get_shader_info_log(fragmentShaderId)).WillOnce(Return("fragment compile error"));
        EXPECT_CALL(mock, delete_shader(fragmentShaderId));
        EXPECT_CALL(mock, delete_shader(vertexShaderId));
    }

    const opengl::ProgramCreationResult result = opengl::create_program(vertexShaderSource, fragmentShaderSource);

    expect_error_code(result, opengl::ProgramCreationFailureStage::FragmentShaderCompilation);
}

TEST_F(CreateProgramTest, ReportsProgramCreationFailure) {
    {
        InSequence sequence;
        expect_successful_vertex_shader_setup(mock);
        expect_successful_fragment_shader_setup(mock);
        EXPECT_CALL(mock, create_program()).WillOnce(Return(0U));
        EXPECT_CALL(mock, delete_shader(fragmentShaderId));
        EXPECT_CALL(mock, delete_shader(vertexShaderId));
    }

    const opengl::ProgramCreationResult result = opengl::create_program(vertexShaderSource, fragmentShaderSource);

    expect_error_code(result, opengl::ProgramCreationFailureStage::ProgramCreation);
}

TEST_F(CreateProgramTest, ReportsProgramLinkFailure) {
    {
        InSequence sequence;
        expect_successful_vertex_shader_setup(mock);
        expect_successful_fragment_shader_setup(mock);
        EXPECT_CALL(mock, create_program()).WillOnce(Return(programId));
        EXPECT_CALL(mock, attach_shader(programId, vertexShaderId));
        EXPECT_CALL(mock, attach_shader(programId, fragmentShaderId));
        EXPECT_CALL(mock, link_program(programId));
        EXPECT_CALL(mock, get_program_link_status(programId)).WillOnce(Return(false));
        EXPECT_CALL(mock, get_program_info_log(programId)).WillOnce(Return("program link error"));
        EXPECT_CALL(mock, delete_program(programId));
        EXPECT_CALL(mock, delete_shader(fragmentShaderId));
        EXPECT_CALL(mock, delete_shader(vertexShaderId));
    }

    const opengl::ProgramCreationResult result = opengl::create_program(vertexShaderSource, fragmentShaderSource);

    expect_error_code(result, opengl::ProgramCreationFailureStage::ProgramLinking);
}
