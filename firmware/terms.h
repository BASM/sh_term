
#define TERM_I2C_PREF 0x48
#define EE_FROM_TERM(termnum) (100+termnum*6)
typedef struct {
	uint8_t num;
	int16_t min;
	int16_t max;
	int16_t cur;
	int8_t stat;
	uint8_t port;
} terms;

#define TERM_MAX (0xffff>>1)
typedef int16_t Term;

#define TERMS 8
extern terms term_base_int;
extern terms term_base_ext;
extern terms terms_extend[]; 

void TermUpdate(terms* T, int16_t cur);
int  TermLoad(terms* T, int num);
void TermSave(terms* T, uint8_t num);
