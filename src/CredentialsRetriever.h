#ifndef CREDENTIALS_RETRIEVER_H
#define CREDENTIALS_RETRIEVER_H

#include <Data.h>

namespace CredentialsRetriever {
namespace {
bool gotCredentials = false;
Credentials credentials;
} // namespace

bool hasCredentials() { return gotCredentials; }
Credentials getCredentials() { return credentials; }

void setCredentials(Credentials new_credentials) {
  credentials = new_credentials;
  gotCredentials = true;
}
void invalidateCredentials() { gotCredentials = false; }

}; // namespace CredentialsRetriever

#endif