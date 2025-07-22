# Contributing to Farm Urban Arduino Sensors

Thank you for your interest in contributing to the Farm Urban Arduino Sensors project! This document provides guidelines and instructions for contributing.

## Table of Contents

- [Code of Conduct](#code-of-conduct)
- [Getting Started](#getting-started)
- [Development Workflow](#development-workflow)
- [Code Style](#code-style)
- [Testing](#testing)
- [Documentation](#documentation)
- [Pull Request Process](#pull-request-process)

## Code of Conduct

By participating in this project, you agree to abide by our Code of Conduct. Please read it before contributing.

## Getting Started

### Prerequisites

- [PlatformIO](https://platformio.org/) installed
- Arduino IDE (optional, for debugging)
- Git
- Python 3.7+ (for testing and scripts)

### Setting Up Development Environment

1. Fork the repository
2. Clone your fork:

   ```bash
   git clone https://github.com/YOUR_USERNAME/fufarm_arduino_sensors.git
   cd fufarm_arduino_sensors
   ```

3. Add the original repository as upstream:

   ```bash
   git remote add upstream https://github.com/farm-urban/fufarm_arduino_sensors.git
   ```

### Building the Project

1. Install dependencies (When using VSCode or a derivative like Cursor this would happen automatically):

   ```bash
   pio pkg install
   ```

2. Build for a specific board:

   ```bash
   pio run --environment esp32e_firebeetle2
   ```

3. Upload to a board:

   ```bash
   pio run --environment esp32e_firebeetle2 --target upload
   ```

## Development Workflow

1. Create a new branch for your feature/fix:

   ```bash
   git checkout -b feature/your-feature-name
   ```

2. Make your changes following the [Code Style](#code-style) guidelines

3. Test your changes:

   ```bash
   pio test --environment native
   ```

4. Commit your changes:

   ```bash
   git commit -m "add new feature"
   ```

5. Push to your fork:

   ```bash
   git push origin feature/your-feature-name
   ```

6. Create a Pull Request

## Code Style

### C++ Guidelines

<!-- - Follow the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html) -->
- Use meaningful variable and function names
- Add comments for complex logic
- Keep functions small and focused
- Use const where appropriate

### Arduino Specific

- Use Arduino's built-in types (e.g., `uint8_t` instead of `unsigned char`)
- Follow the pin naming convention in `platformio.ini`
- Document pin assignments in code comments

## Testing

### Unit Tests

- Write tests for new features
- Ensure all tests pass before submitting PR
- Run tests with:

  ```bash
  pio test --environment native
  ```

### Hardware Testing

- Test on at least one supported board
- Document any hardware-specific considerations
- Include test results in PR description

## Documentation

### Code Documentation

- Document all public functions and classes
- Include examples for complex functionality
- Update README.md for significant changes

### Sensor Documentation

- Document pin assignments
- Include calibration procedures
- Add troubleshooting guides

## Pull Request Process

1. Update the README.md with details of changes if needed
2. Update the version.txt file
3. Ensure all tests pass
4. Request review from maintainers
5. Address any feedback
6. Wait for approval and merge

## Adding New Sensors

1. Add sensor configuration to `platformio.ini`
2. Create sensor interface in `src/sensors.h`
3. Implement sensor reading in `src/sensors.cpp`
4. Add calibration support if needed
5. Update documentation
6. Add tests

## Troubleshooting

### Common Issues

- Build failures: Check board compatibility
- Upload errors: Verify board connection
- Sensor reading issues: Check calibration

### Getting Help

- Open an issue for bugs
- Use discussions for questions

## License

By contributing, you agree that your contributions will be licensed under the project's [LICENSE](LICENSE) file.
