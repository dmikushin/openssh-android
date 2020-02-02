// Call an external program /system/bin/getaddrinfo
// to resolve hostnames on Android. This is ugly, but avoids
// painful compilation of OpenSSH with Android toolchain.
#ifdef __ANDROID__

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <malloc.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h> 
#include <unistd.h>

int getaddrinfo_android(
	const char* node, const char* service,
	const struct addrinfo* hints, struct addrinfo** res)
{
	if (!node)
	{
		fprintf(stderr, "getaddrinfo_android: Invalid argument \"node\"\n");
		return EAI_FAIL;
	}

	if (!service)
	{
		fprintf(stderr, "getaddrinfo_android: Invalid argument \"service\"\n");
		return EAI_FAIL;
	}

	if (!res)
	{
		fprintf(stderr, "getaddrinfo_android: Invalid argument \"res\"\n");
		return EAI_FAIL;
	}

	if (!system(NULL))
	{
		fprintf(stderr, "getaddrinfo_android: The command line processor is not available\n");
		return EAI_FAIL;
	}

	char filename[] = "/data/local/tmp/XXXXXX";
	int fd = mkstemp(filename);
	if (fd < 1)
	{
		fprintf(stderr, "getaddrinfo_android: Could not create a temporary file %s\n", filename);
		return EAI_FAIL;
	}
	unlink(filename);

	int command_printed_size = snprintf(NULL, 0,
		"%s "  // resolv
		"%s "  // node
		">%s", // filename
		"/system/bin/nslookup",
		node, filename);
	if (command_printed_size < 0)
	{
		fprintf(stderr, "getaddrinfo_android: Could not encode the command\n");
		return EAI_FAIL;
	}
	char* command = malloc(command_printed_size + 1);
	sprintf(command,
		"%s "  // nslookup
		"%s "  // node
		">%s", // filename
		"/system/bin/nslookup",
		node, filename);

#if 0
	printf("command = %s\n", command);
#endif

	// Call the command line resolver utility.
	int err = system(command);
	if (err)
	{
		fprintf(stderr, "getaddrinfo_android: The resolver command line utility retuned non-zero error code %d\n", err);
		return EAI_FAIL;
	}
	free(command);

	FILE* fp = fopen(filename, "r");
	if (!fp)
	{
		fprintf(stderr, "getaddrinfo_android: Could not read the temporary file %s\n", filename);
		return EAI_FAIL;
	}

	// Read the response temporary file to get the result
	*res = malloc(1);
	int nres = 0;
	char* line = NULL;
	size_t len = 0;
	ssize_t read;
	char* name = NULL;
	char* address = NULL;
	while ((read = getline(&line, &len, fp)) != -1)
	{
		if (strncmp(line, "Name:", strlen("Name:")) == 0)
		{
			line[read - 1] = '\0';
			char* p = line + strlen("Name:");
			while ((*p == ' ') || (*p == '\t')) p++;
			if (!name) free(name);
			name = strdup(p);
		}
		else if (strncmp(line, "Address:", strlen("Address:")) == 0)
		{
			line[read - 1] = '\0';
			char* p = line + strlen("Address:");
			while ((*p == ' ') || (*p == '\t')) p++;
			if (!address) free(address);
			address = strdup(p);
		}
		
		if (!(name && address))
			continue;

		nres++;
		*res = realloc(*res, nres * sizeof((*res)[0]));
		struct addrinfo* r = &(*res)[nres - 1];
		memset(r, 0, sizeof((*res)[0]));
		
		if (nres > 1)
			(*res)[nres - 2].ai_next = r;

		r->ai_socktype = SOCK_STREAM;
		r->ai_flags |= AI_CANONNAME;

		r->ai_canonname = name;
		name = NULL;

		char buffer[sizeof(struct in6_addr)];
		if (inet_pton(AF_INET, address, buffer) == 1)
		{
			r->ai_family = AF_INET;
			r->ai_addrlen = sizeof(struct sockaddr_in);
			struct sockaddr_in* addr = malloc(sizeof(struct sockaddr_in));
			memset(addr, 0, sizeof(struct sockaddr_in));
			addr->sin_family = AF_INET;
			addr->sin_port = htons(atoi(service));
			memcpy(&addr->sin_addr, buffer, sizeof(struct in_addr));
			r->ai_addr = addr;
		}
		else if (inet_pton(AF_INET6, address, buffer) == 1)
		{
			r->ai_family = AF_INET6;
			r->ai_addrlen = sizeof(struct sockaddr_in6);
			struct sockaddr_in6* addr = malloc(sizeof(struct sockaddr_in6));
			memset(addr, 0, sizeof(struct sockaddr_in6));
			addr->sin6_family = AF_INET6;
			addr->sin6_port = htons(atoi(service));
			memcpy(&addr->sin6_addr, buffer, sizeof(struct in6_addr));
			r->ai_addr = addr;
		}
		else
		{
			fprintf(stderr, "getaddrinfo_android: Invalid address value \"%s\"\n", address);
				
			free(address);
			free(*res);
			return EAI_FAIL;
		}
		
		free(address);
		address = NULL;
	}

	fclose(fp);
	close(fd);
	
	if (!nres)
	{
		free(*res);
		*res = NULL;
	}
	
	return 0;
}

#endif // __ANDROID__

