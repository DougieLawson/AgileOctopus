/* 
 Copyright © Dougie Lawson, 2020-2021, All rights reserved 
*/

#include <stdio.h>     /* for printf */
#include <stdlib.h>    /* for exit */
#include <string.h>
#include <getopt.h>
#include <libconfig.h>
#include <curl/curl.h>
#include <sys/stat.h>
#include <pcre.h>

#define CONFIG_FILE "/home/pi/.zappi.cfg"
#define MYENERGI_URL "https://director.myenergi.net"

#define FALSE 0
#define TRUE !(FALSE)

const char* matchedString;
int offsetVector[30];

int findMy(void* testString)
{
	char* regexPattern;
	pcre* regexComp;
	pcre_extra* regExtra;
	int regexErrorCode;
	int regexErrorOffset;
	const char* regexErrorStr;
	int j;

	regexPattern = "x_myenergi-asn: (.*.myenergi.net)";
	regexComp = pcre_compile(regexPattern, PCRE_CASELESS, &regexErrorStr, &regexErrorOffset, NULL);
	if (regexComp == NULL) 
	{
		printf("Error: %s %s\n", regexPattern, regexErrorStr);
		exit(1);
	}

	regExtra = pcre_study(regexComp, 0, &regexErrorStr);
	if (regexErrorStr != NULL)
	{
		printf("Study: %s %s\n", regexPattern, regexErrorStr);
		exit(1);
	}
	regexErrorCode = pcre_exec(regexComp, regExtra, testString,
		strlen(testString), 0, 0, offsetVector, 30);
	if (regexErrorCode > 0)
	{
		pcre_get_substring(testString, offsetVector, regexErrorCode, 1, &(matchedString));
	}
	else if (regexErrorCode < 0)
	{
		switch(regexErrorCode)
		{
			case PCRE_ERROR_NOMATCH : 
				//printf("No match\n"); 
				break;
			case PCRE_ERROR_NULL : 
				printf("Null\n"); 
				break;
			case PCRE_ERROR_BADOPTION : 
				printf("Bad option\n"); 
				break;
			case PCRE_ERROR_BADMAGIC : 
				printf("Voodoo\n"); 
				break;
			case PCRE_ERROR_UNKNOWN_NODE : 
				printf("Something kooky\n"); 
				break;
			case PCRE_ERROR_NOMEMORY : 
				printf("Oom\n"); 
				break;
			default : printf("Very bad\n"); break;
		}
	}
	else
	{
		printf("No error, but no match");
	}
	pcre_free(regexComp);
	if (regExtra != NULL)
	{
		pcre_free_study(regExtra);
	}
	return 0;
}

static size_t curl_print(void* ptr, size_t size, size_t nmemb, void* stream)
{
	size_t written =  fwrite(ptr, size, nmemb, (FILE*)stream);
	//size_t realsize = size * nmemb;
	return written; 
}

static size_t curl_header(void* ptr, size_t size, size_t nitems, void* userdata)
{
	findMy(ptr);
	return nitems * size;
}

void getASN(config_setting_t* z_user, config_setting_t* z_pwd)
{
	CURL* curl;
	CURLcode res;
	static const char *outfilenm = "/tmp/curl.out";
	const char* username;
	const char* password;
	FILE* outfile;

	username = config_setting_get_string(z_user);
	password = config_setting_get_string(z_pwd);

	curl_global_init(CURL_GLOBAL_ALL);
 
	curl = curl_easy_init();
	if(curl) {

		curl_easy_setopt(curl, CURLOPT_USERNAME, username);
		curl_easy_setopt(curl, CURLOPT_PASSWORD, password);
		curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST);
		curl_easy_setopt(curl, CURLOPT_HEADER, 1);
		curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, curl_header);
		curl_easy_setopt(curl, CURLOPT_URL, MYENERGI_URL);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_print);

		outfile = fopen(outfilenm, "wb");
		if (outfile) {
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, outfile);

    			res = curl_easy_perform(curl);
   		 	if(res != CURLE_OK)
				fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
			fclose(outfile);
		}
	curl_easy_cleanup(curl);
	}
 
	curl_global_cleanup();
 
	printf("Match: (%2d, %2d): %s\n", offsetVector[2], offsetVector[3], matchedString);
}

int main(int argc, char **argv)
{
	int c;
	int digit_optind = 0;
	static const char *output_file = CONFIG_FILE;
	mode_t mode;
	config_t cfg;
	config_setting_t *root, *setting;
	config_setting_t *zappi, *z_user, *z_pwd, *z_asn;
	config_setting_t *database, *d_host, *d_db, *d_pwd, *d_user;
	config_setting_t *octopus, *o_apikey, *o_mpan, *o_serial, *o_region;
	config_init(&cfg);
	root = config_root_setting(&cfg);
	_Bool u_OK, p_OK, h_OK, d_OK, q_OK, s_OK, a_OK, m_OK, e_OK, r_OK, required;
	u_OK = FALSE;
	p_OK = FALSE;
	h_OK = FALSE;
	d_OK = FALSE;
	q_OK = FALSE;
	s_OK = FALSE;
	a_OK = FALSE;
	m_OK = FALSE;
	e_OK = FALSE;
	r_OK = FALSE;
	required = FALSE;

	zappi = config_setting_add(root, "Zappi", CONFIG_TYPE_GROUP);
	database = config_setting_add(root, "Database", CONFIG_TYPE_GROUP);
	octopus = config_setting_add(root, "Octopus", CONFIG_TYPE_GROUP);

	while (1)
	{
		int this_option_optind = optind ? optind : 1;
		int option_index = 0;
		static struct option long_options[] = {
			{"username", optional_argument, 0,  0 },
			{"password", optional_argument, 0,  0 },
			{"sqluser", required_argument, 0,  0 },
			{"sqlpwd", required_argument, 0,  0 },
			{"sqlhost", required_argument, 0,  0 },
			{"sqldbase", required_argument, 0,  0 },
			{"apikey", optional_argument, 0, 0},
			{"mpan", optional_argument, 0, 0},
			{"serial", optional_argument, 0, 0},
			{"region", optional_argument, 0, 0},
			{0,          0,                 0,  0 }
		};
		c = getopt_long(argc, argv, "u:p:h:d:q:s:a:m:e:r:", long_options, &option_index);
		if (c == -1) break;
		switch (c)
		{
			case 0:
				printf("option --%s", long_options[option_index].name);

				if (!strcmp(long_options[option_index].name, "username")) u_OK = TRUE;
				else if (!strcmp(long_options[option_index].name, "password")) p_OK = TRUE;
				else if (!strcmp(long_options[option_index].name, "sqluser")) s_OK = TRUE;
				else if (!strcmp(long_options[option_index].name, "sqlhost")) h_OK = TRUE;
				else if (!strcmp(long_options[option_index].name, "sqldbase")) d_OK = TRUE;
				else if (!strcmp(long_options[option_index].name, "sqlpwd")) q_OK = TRUE;
				else if (!strcmp(long_options[option_index].name, "apikey")) a_OK = TRUE;
				else if (!strcmp(long_options[option_index].name, "mpan")) m_OK = TRUE;
				else if (!strcmp(long_options[option_index].name, "serial")) s_OK = TRUE;
				else if (!strcmp(long_options[option_index].name, "region")) r_OK = TRUE;

				if (optarg) printf(" with value %s", optarg);
				if (option_index == 0 || option_index == 1)
				{
					setting = config_setting_add(zappi, long_options[option_index].name, CONFIG_TYPE_STRING);
				}
				else if (option_index >= 2 && option_index <= 5)
				{
					setting = config_setting_add(database, long_options[option_index].name, CONFIG_TYPE_STRING);
				}
				else
				{
					setting = config_setting_add(octopus, long_options[option_index].name, CONFIG_TYPE_STRING);
				}
				config_setting_set_string(setting, optarg);
				printf("\n");
				break;
			case 'u':
				printf("Zappi user: -u with value '%s'\n", optarg);
				z_user = config_setting_add(zappi, "username", CONFIG_TYPE_STRING);
				config_setting_set_string(z_user, optarg);
				u_OK = TRUE;
				break;
			case 'p':
				printf("Zappi password: -p with value '%s'\n", optarg);
				z_pwd = config_setting_add(zappi, "password", CONFIG_TYPE_STRING);
				config_setting_set_string(z_pwd, optarg);
				p_OK = TRUE;
				break;
			case 'h':
				printf("SQL host: -h with value '%s'\n", optarg);
				d_host = config_setting_add(database, "sqlhost", CONFIG_TYPE_STRING);
				config_setting_set_string(d_host, optarg);
				h_OK = TRUE;
				break;
			case 'd':
				printf("SQL database: -d with value '%s'\n", optarg);
				d_db = config_setting_add(database, "sqldbase", CONFIG_TYPE_STRING);
				config_setting_set_string(d_db, optarg);
				d_OK = TRUE;
				break;
			case 'q':
				printf("SQL password: -q with value '%s'\n", optarg);
				d_pwd = config_setting_add(database, "sqlpwd", CONFIG_TYPE_STRING);
				config_setting_set_string(d_pwd, optarg);
				q_OK = TRUE;
				break;
			case 's':
				printf("SQL user: -s with value '%s'\n", optarg);
				d_user = config_setting_add(database, "sqluser", CONFIG_TYPE_STRING);
				config_setting_set_string(d_user, optarg);
				s_OK = TRUE;
				break;
			case 'a':
				printf("Octopus API key: -a with value '%s'\n", optarg);
				o_apikey = config_setting_add(octopus, "api_key", CONFIG_TYPE_STRING);
				config_setting_set_string(o_apikey, optarg);
				a_OK = TRUE;
				break;
			case 'm':
				printf("MPAN: -e with value '%s'\n", optarg);
				o_mpan = config_setting_add(octopus, "elec_mpan", CONFIG_TYPE_STRING);
				config_setting_set_string(o_mpan, optarg);
				e_OK = TRUE;
				break;
			case 'e':
				printf("Meter serial: -m with value '%s'\n", optarg);
				o_serial = config_setting_add(octopus, "elec_ser", CONFIG_TYPE_STRING);
				config_setting_set_string(o_serial, optarg);
				m_OK = TRUE;
				break;
			case 'r':
				printf("DNO region: -r with value '%s'\n", optarg);
				o_region = config_setting_add(octopus, "region", CONFIG_TYPE_STRING);
				config_setting_set_string(o_region, optarg);
				r_OK = TRUE;
				break;
			case '?':
				break;
			default:
				printf("?? getopt returned character code 0%o ??\n", c);
		}
	}
	if (optind < argc)
	{
		printf("non-option ARGV-elements: ");
		while (optind < argc)
		printf("%s ", argv[optind++]);
		printf("\n");
	}
	if (u_OK == FALSE || p_OK == FALSE ||
            h_OK == FALSE || d_OK == FALSE || q_OK == FALSE || s_OK == FALSE ||
	    a_OK == FALSE || e_OK == FALSE || m_OK == FALSE || r_OK == FALSE)
	{
		required = FALSE;
		fprintf(stderr, "Missing option(s): ");
		if (!u_OK)
		{
		       	fprintf(stderr, "\nZappi userid. Use: -uXXX or --username XXX ");
		}
		if (!p_OK)
		{
			fprintf(stderr, "\nZappi password. Use:  -pXXX or --password XXX ");
		}
		if (!h_OK)
		{	
			fprintf(stderr, "\nMariadb hostname or IP address. Use: -h192.168.3.14  or --sqlhost raspberrypi.local ");
			required = TRUE;
		}
		if (!d_OK)
		{
		       	fprintf(stderr, "\nMariadb database name. Use: -dXXX or --sqldbase XXX ");
			required = TRUE;
		}
		if (!q_OK)
		{	
			fprintf(stderr, "\nMariadb password. Use: -qXXX or --sqlpwd XXX ");
			required = TRUE;
		}
		if (!s_OK)
		{	
			fprintf(stderr, "\nMariadb userid. Use -sXXXX or --sqluser XXX ");
			required = TRUE;
		}
		if (!a_OK)
		{	
			fprintf(stderr, "\nOctopus API (optional). Use -aXXX or --apikey XXX");
		}
		if (!e_OK)
		{	
			fprintf(stderr, "\nOctopus API (optional). Use -eXXX or --serial XXX");
		}
		if (!m_OK)
		{
		       	fprintf(stderr, "\nOctopus API (optional). Use -mXXX or --mpan XXX");
		}
		if (!r_OK)
		{	
			fprintf(stderr, "\nOctopus API (optional). Use -rXXX or --region XXX");
		}
		fprintf(stderr, "\n");
		if (required) exit(20);
	}
	if (u_OK && p_OK)
	{
		getASN(z_user, z_pwd);
		z_asn = config_setting_add(zappi, "asn", CONFIG_TYPE_STRING);
		config_setting_set_string(z_asn, matchedString);
	}
	else 
	{
		fprintf(stderr, "\nMissing Myenergi credentials - asn not available");
	}
	if(! config_write_file(&cfg, output_file))
	{
		fprintf(stderr, "Error while writing file.\n");
		config_destroy(&cfg);
		return(EXIT_FAILURE);
	}

	fprintf(stderr, "New configuration successfully written to: %s\n", output_file);
	config_destroy(&cfg);
	mode = 0600;
	chmod(output_file, mode);
	exit(EXIT_SUCCESS);
}
