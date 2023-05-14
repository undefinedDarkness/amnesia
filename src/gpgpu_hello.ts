const adapter = await navigator.gpu.requestAdapter()
adapter!.limits.maxComputeWorkgroupsPerDimension=0;
const gpu = await adapter?.requestDevice()!;
const gpuInfo = await adapter?.requestAdapterInfo()
console.log(gpuInfo, gpu)


// const testingA = gpu.createBuffer({
// 	mappedAtCreation: false,
// 	usage: GPUBufferUsage.STORAGE | GPUBufferUsage.COPY_SRC,
// 	size:  4 * 1024 // 128 integers
// })

// const bindGroup = gpu.createBindGroup({
// 	layout: gpu.createBindGroupLayout({
// 		entries: [
// 			{
// 				binding: 0,
// 				visibility: GPUShaderStage.COMPUTE,
// 				buffer: {
// 					type: "storage"
// 				}
// 			}
// 		]
// 	}),
// 	entries: [
// 		{
// 			binding: 0,
// 			resource: {
// 				buffer: testingA
// 			}
// 		}
// 	]
// })

// const shaderModule = gpu.createShaderModule({
// 	code: `

// 	var<private> rand_state : i32;
// 	@group(0) @binding(0) var<storage, read_write> data : array<u32>;
	
// 	fn rand() -> u32 {
// 		rand_state ^= (rand_state << 13);
// 		rand_state ^= (rand_state >> 17);
// 		rand_state ^= (rand_state << 5);
// 		return rand_state;
// 	}
	

// 	@compute 
// 	@workgroup_size(64, 16) // 8x8 grid of invocations
// 	fn main(@builtin(global_invocation_id) global_id : vec3u, @builtin(local_invocation_id) local_id: vec3u) {
// 		rand_state = local_id.x * local_id.y;
// 		data[local_id.y * 64 + local_id.x] = rand();	
// 	}

// 	`
// })

// const computePipeline = gpu.createComputePipeline({
// 	layout: gpu.createPipelineLayout({
// 		bindGroupLayouts: [bindGroup]
// 	}),
// 	compute: {
// 		module: shaderModule,
// 		entryPoint: 'main'
// 	}
// })

// const commandEncoder = gpu.createCommandEncoder();
// const passEncoder= commandEncoder.beginComputePass();
// passEncoder.setPipeline(computePipeline);
// passEncoder.setBindGroup(0, bindGroup);
// passEncoder.dispatchWorkgroups(1)
// passEncoder.end();

// const readBuffer = gpu.createBuffer({
// 	size: 1024 * 4,
// 	usage: GPUBufferUsage.COPY_DST | GPUBufferUsage.MAP_READ
// })

// commandEncoder.copyBufferToBuffer(testingA, 0, readBuffer, 0, 1024*4)
// await readBuffer.mapAsync(GPUMapMode.READ);
// const values = new Uint32Array(readBuffer.getMappedRange())
// for (let i = 0; i < 512; i++) {
// 	console.log(values[i])
// }
