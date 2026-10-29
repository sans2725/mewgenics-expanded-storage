#ifndef ES_VAULT_H
#define ES_VAULT_H
#include <stddef.h>
#include <stdint.h>
#ifdef _WIN32
#define ES_API __declspec(dllexport)
#else
#define ES_API
#endif

/* This envelope owns no game objects. Payload must come from a verified native
 * bucket serializer, never an in-memory C++ object or a guessed item layout. */
#define ES_HEADER_BYTES 96u
#define ES_MAX_PAYLOAD (16u * 1024u * 1024u)
#define ES_MAX_CAPACITY 2048u
#define ES_DEFAULT_CAPACITY 600u

typedef enum {
    ES_OK, ES_ARGUMENT, ES_BOUNDS, ES_FORMAT, ES_CHECKSUM,
    ES_WRONG_BUILD, ES_WRONG_CAMPAIGN, ES_STATE, ES_STALE, ES_UNCERTAIN
} ESResult;
typedef struct {
    uint8_t build[32];
    uint8_t campaign[16];
    uint64_t revision;
    uint32_t capacity, count;
    const uint8_t *payload;
    uint32_t payload_bytes;
} ESVault;
ES_API const char *es_result_name(ESResult result);
ES_API ESResult es_vault_encode(const ESVault *vault, uint8_t *out,
                                size_t out_bytes, size_t *written);
/* Output borrows input memory. On any error output remains zeroed. */
ES_API ESResult es_vault_decode(const uint8_t *data, size_t bytes,
                                const uint8_t expected_build[32],
                                const uint8_t expected_campaign[16], ESVault *out);

/* A transfer must commit BOTH native inventory and vault in the same durable
 * transaction. These gates do not implement that adapter or certify its safety. */
typedef enum { ES_DETACHED, ES_READY, ES_PREPARED, ES_FROZEN } ESPhase;
typedef struct {
    ESPhase phase;
    uint8_t campaign[16];
    uint64_t revision;
} ESTransaction;
typedef enum { ES_COMMITTED, ES_ROLLED_BACK, ES_OUTCOME_UNKNOWN } ESOutcome;
ES_API ESResult es_tx_attach(ESTransaction *tx, const uint8_t campaign[16], uint64_t revision);
ES_API ESResult es_tx_prepare(ESTransaction *tx, const uint8_t campaign[16], uint64_t expected_revision);
ES_API ESResult es_tx_finish(ESTransaction *tx, ESOutcome outcome);
ES_API ESResult es_tx_detach(ESTransaction *tx);
#endif
