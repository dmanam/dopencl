Commands that include data transfers (read, write, map, unmap), as all commands,
are controlled by the device or compute node, respectively, that executes that
command.
The most efficient way to implement this behavior is Remote Direct Memory Access
(RDMA), which allows the compute node to access (read, write) the host's memory
without any participation from the host.

However, in the current implementation of dOpenCL, RDMA is not supported.
Therefore, data transfers require participation from the host. Command objects
represent these data transfers, and the operations the host has to perform in
order to facilitate the corresponding data transfer.
Commands are a work-around that replaces RDMA and are only required for the
current implementation of the dOpenCL communication library, but not for dOpenCL
in general, the command classes should be provided by the dOpenCL communication
library rather than the ICD. Even if the underlying network does not support
RDMA, dOpenCL thus will provide RDMA-like API calls for data transfers.
