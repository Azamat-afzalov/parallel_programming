# Use a lightweight Linux base image
FROM ubuntu:22.04

# Install necessary tools
RUN apt-get update && apt-get install -y \
    fuse \
    libfuse2 \
    wget \
    && rm -rf /var/lib/apt/lists/*

# Set up FUSE (required for AppImage execution)
RUN echo "user_allow_other" >> /etc/fuse.conf

# Set the default working directory
WORKDIR /app

# Copy your AppImage into the container
# Replace `your-appimage.AppImage` with your actual AppImage file name
COPY EduMPI.AppImage /app/

# Make the AppImage executable
RUN chmod +x EduMPI.AppImage

# Default command to run the AppImage
CMD ["./EduMPI.AppImage"]
