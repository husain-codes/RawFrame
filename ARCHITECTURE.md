## System Architecture

This diagram illustrates the control and data planes from the bare-metal hardware up through the Linux Kernel into the userspace C/C++ engine.

```mermaid
graph TD
    subgraph Hardware["Hardware Layer (Raspberry Pi & Sensor)"]
        Sensor[Camera Sensor<br>e.g., IMX219]
        MIPI((MIPI CSI-2<br>Data Lanes))
        I2C_Bus((I2C Bus<br>Control))
    end

    subgraph Kernel["Linux Kernel Space"]
        I2C_Driver[Sensor Driver<br>/dev/v4l-subdev0]
        CSI_RX[Unicam CSI-2 Receiver]
        V4L2[V4L2 Framework<br>/dev/video0]
        DMA_Mem[(CMA / DMA Memory)]
    end

    subgraph Userspace["Userspace (RawFrame & C++ App)"]
        Capture[Capture Thread<br>V4L2 ioctls & poll]
        ISP[RawFrame C Library<br>ISP Pipeline]
        App[C++ Render Engine<br>State & Concurrency]
        GPU[OpenGL / EGL<br>Texture Mapping]
    end

    %% Control Flow (Dotted lines)
    App -.->|V4L2 Controls - Exposure, Gain| V4L2
    V4L2 -.-> I2C_Driver
    I2C_Driver -.-> I2C_Bus
    I2C_Bus -.-> Sensor

    %% Data Flow (Thick lines)
    Sensor == Raw Bayer Bytes ==> MIPI
    MIPI == High-Speed Packets ==> CSI_RX
    CSI_RX == Hardware DMA Write ==> DMA_Mem
    DMA_Mem == mmap / dma-buf ==> Capture
    Capture == Lock-free Queue ==> ISP
    ISP == Processed RGB ==> App
    App == Zero-Copy Upload ==> GPU
    
    classDef hardware fill:#2d3436,stroke:#b2bec3,stroke-width:2px,color:#dfe6e9;
    classDef kernel fill:#0984e3,stroke:#74b9ff,stroke-width:2px,color:#fff;
    classDef userspace fill:#00b894,stroke:#55efc4,stroke-width:2px,color:#fff;
    
    class Sensor,MIPI,I2C_Bus hardware;
    class I2C_Driver,CSI_RX,V4L2,DMA_Mem kernel;
    class Capture,ISP,App,GPU userspace;
