#!/bin/sh

set -ex

if [ "${TRAVIS_OS_NAME}" = "linux" ] && [ ! -z "${DOCKER_IMG}" ]; then
  CHANGED=`git diff --name-only HEAD~1 HEAD | grep ci/${DOCKER_IMG}/Dockerfile | wc -l`

  if [ "${CHANGED}" = "1" ] && [ "${TRAVIS_PULL_REQUEST}" = "false" ]; then
    echo "Pushing new image to Docker Hub!"
    docker login -u "$DOCKER_USER" -p "$DOCKER_PASSWORD"
    docker push ${DOCKER_USER}/keepassxc:${DOCKER_IMG}-${CONFIG}
  fi
fi
