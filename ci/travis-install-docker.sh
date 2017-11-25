CHANGED=`git diff --name-only | grep ci/${DOCKER_IMAGE}/Dockerfile | wc -l`
AVAILABLE=`curl --silent https:/hub.docker.com/r/${DOCKER_USER}/keepassxc/tags | grep ${DOCKER_IMG}-${CONFIG} | wc -l`

if [ "${CHANGED}" = "0" ] && [ "${AVAILABLE}" = "1" ]; then
  status=docker pull ${DOCKER_USER}/keepassxc:${DOCKER_IMG}-${CONFIG};
else
  docker build -t ${DOCKER_USER}/keepassxc:${DOCKER_IMG}-${CONFIG} ci/${DOCKER_IMG};
fi
